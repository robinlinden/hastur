// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "wasm/wasm.h"

#include "etest/etest.h"

#include <tl/expected.hpp>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iterator>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace std::literals;

using etest::expect_eq;

namespace {

// https://webassembly.github.io/spec/core/binary/modules.html#sections
enum class SectionId {
    Custom = 0,
    Type = 1,
    Import = 2,
    Function = 3,
    Table = 4,
    Memory = 5,
    Global = 6,
    Export = 7,
    Start = 8,
    Element = 9,
    Code = 10,
    Data = 11,
    DataCount = 12,
};

std::stringstream make_module_bytes(SectionId id, std::vector<std::uint8_t> const &section_content) {
    std::stringstream wasm_bytes;
    wasm_bytes << "\0asm\1\0\0\0"sv;
    wasm_bytes << static_cast<std::uint8_t>(id);
    assert(section_content.size() < 0x7f); // > 0x7f requires leb128-serialization.
    wasm_bytes << static_cast<std::uint8_t>(section_content.size());
    std::ranges::copy(section_content, std::ostreambuf_iterator<char>{wasm_bytes});
    return wasm_bytes;
}

void export_section_tests() {
    etest::test("export section, missing export count", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Export, {}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidExportSection});
    });

    etest::test("export section, missing export after count", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Export, {1}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidExportSection});
    });

    etest::test("export section, empty", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Export, {0})).value();
        expect_eq(module.export_section, wasm::ExportSection{});
    });

    etest::test("export section, one", [] {
        std::vector<std::uint8_t> content{1, 2, 'h', 'i', static_cast<std::uint8_t>(wasm::Export::Type::Function), 5};

        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Export, content)).value();
        expect_eq(module.export_section,
                wasm::ExportSection{.exports{wasm::Export{"hi", wasm::Export::Type::Function, 5}}});
    });

    etest::test("export section, two", [] {
        std::vector<std::uint8_t> content{
                2,
                2,
                'h',
                'i',
                static_cast<std::uint8_t>(wasm::Export::Type::Function),
                5,
                3,
                'l',
                'o',
                'l',
                static_cast<std::uint8_t>(wasm::Export::Type::Global),
                2,
        };

        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Export, content)).value();
        expect_eq(module.export_section,
                wasm::ExportSection{.exports{
                        wasm::Export{"hi", wasm::Export::Type::Function, 5},
                        wasm::Export{"lol", wasm::Export::Type::Global, 2},
                }});
    });

    etest::test("export section, missing name", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Export, {1, 2}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidExportSection});
    });

    etest::test("export section, missing type", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Export, {1, 1, 'a'}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidExportSection});
    });

    etest::test("export section, missing index", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Export, {1, 1, 'a', 1}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidExportSection});
    });
}

void start_section_tests() {
    etest::test("start section, missing start", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Start, {}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidStartSection});
    });

    etest::test("start section, excellent", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Start, {42})).value();
        expect_eq(module.start_section, wasm::StartSection{.start = 42});
    });
}

void function_section_tests() {
    etest::test("function section, missing data", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Function, {}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidFunctionSection});
    });

    etest::test("function section, empty", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Function, {0})).value();
        expect_eq(module.function_section, wasm::FunctionSection{});
    });

    etest::test("function section, missing type indices after count", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Function, {1}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidFunctionSection});
    });

    etest::test("function section, good one", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Function, {2, 9, 13})).value();
        expect_eq(module.function_section, wasm::FunctionSection{.type_indices{9, 13}});
    });
}

void table_section_tests() {
    etest::test("table section, missing data", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Table, {}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidTableSection});
    });

    etest::test("table section, empty", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Table, {0})).value();
        expect_eq(module.table_section, wasm::TableSection{});
    });

    etest::test("table section, no element type", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Table, {1}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidTableSection});
    });

    etest::test("table section, invalid element type", [] {
        constexpr std::uint8_t kInt32Type = 0x7f;
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Table, {1, kInt32Type}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidTableSection});
    });

    static constexpr std::uint8_t kFuncRefType = 0x70;
    static constexpr std::uint8_t kExtRefType = 0x6f;

    etest::test("table section, missing limits", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Table, {1, kFuncRefType}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidTableSection});
    });

    etest::test("table section, invalid has_max in limits", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Table, {1, kFuncRefType, 4}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidTableSection});
    });

    etest::test("table section, missing min in limits", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Table, {1, kFuncRefType, 0}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidTableSection});
    });

    etest::test("table section, only min", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Table, {1, kFuncRefType, 0, 42})).value();
        expect_eq(module.table_section,
                wasm::TableSection{.tables{
                        wasm::TableType{
                                wasm::ValueType{wasm::ValueType::FunctionReference},
                                wasm::Limits{.min = 42},
                        },
                }});
    });

    etest::test("table section, missing max in limits", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Table, {1, kExtRefType, 1, 42}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidTableSection});
    });

    etest::test("table section, min and max", [] {
        auto module =
                wasm::Module::parse_from(make_module_bytes(SectionId::Table, {1, kExtRefType, 1, 42, 42})).value();
        expect_eq(module.table_section,
                wasm::TableSection{.tables{
                        wasm::TableType{
                                wasm::ValueType{wasm::ValueType::ExternReference},
                                wasm::Limits{.min = 42, .max = 42},
                        },
                }});
    });
}

void memory_section_tests() {
    etest::test("memory section, missing data", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Memory, {}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidMemorySection});
    });

    etest::test("memory section, empty", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Memory, {0})).value();
        expect_eq(module.memory_section, wasm::MemorySection{});
    });

    etest::test("memory section, missing limits", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Memory, {1}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidMemorySection});
    });

    etest::test("memory section, invalid has_max in limits", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Memory, {1, 4}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidMemorySection});
    });

    etest::test("memory section, missing min in limits", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Memory, {1, 0}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidMemorySection});
    });

    etest::test("memory section, only min", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Memory, {1, 0, 42})).value();
        expect_eq(module.memory_section,
                wasm::MemorySection{.memories{
                        wasm::MemType{.min = 42},
                }});
    });

    etest::test("memory section, missing max in limits", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Memory, {1, 1, 42}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidMemorySection});
    });

    etest::test("memory section, min and max", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Memory, {1, 1, 42, 42})).value();
        expect_eq(module.memory_section,
                wasm::MemorySection{.memories{
                        wasm::Limits{.min = 42, .max = 42},
                }});
    });

    etest::test("memory section, two memories", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Memory, {2, 1, 4, 51, 1, 19, 84})).value();
        expect_eq(module.memory_section,
                wasm::MemorySection{.memories{
                        wasm::Limits{.min = 4, .max = 51},
                        wasm::Limits{.min = 19, .max = 84},
                }});
    });
}

void type_section_tests() {
    etest::test("type section, missing type data", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Type, {}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidTypeSection});
    });

    etest::test("type section, empty", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Type, {0})).value();
        expect_eq(module.type_section, wasm::TypeSection{});
    });

    etest::test("type section, missing type after count", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Type, {1}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidTypeSection});
    });

    etest::test("type section, bad magic in function type", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Type, {1, 0x59}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidTypeSection});
    });

    etest::test("type section, one type with no parameters and no results", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Type, {1, 0x60, 0, 0})).value();
        expect_eq(module.type_section, wasm::TypeSection{.types{wasm::FunctionType{}}});
    });

    etest::test("type section, eof in parameter parsing", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Type, {1, 0x60, 1}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidTypeSection});
    });

    etest::test("type section, eof in result parsing", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Type, {1, 0x60, 0, 1}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidTypeSection});
    });

    etest::test("type section, two types", [] {
        constexpr std::uint8_t kInt32Byte = 0x7f;
        constexpr std::uint8_t kFloat64Byte = 0x7c;
        auto module = wasm::Module::parse_from(
                make_module_bytes(
                        SectionId::Type, {2, 0x60, 0, 1, kInt32Byte, 0x60, 2, kInt32Byte, kInt32Byte, 1, kFloat64Byte}))
                              .value();

        expect_eq(module.type_section,
                wasm::TypeSection{
                        .types{
                                wasm::FunctionType{.results{wasm::ValueType{wasm::ValueType::Int32}}},
                                wasm::FunctionType{
                                        .parameters{wasm::ValueType{wasm::ValueType::Int32},
                                                wasm::ValueType{wasm::ValueType::Int32}},
                                        .results{wasm::ValueType{wasm::ValueType::Float64}},
                                },
                        },
                });
    });

    etest::test("type section, all types", [] {
        auto module = wasm::Module::parse_from(
                make_module_bytes(SectionId::Type, {1, 0x60, 7, 0x7f, 0x7e, 0x7d, 0x7c, 0x7b, 0x70, 0x6f, 0}))
                              .value();

        expect_eq(module.type_section,
                wasm::TypeSection{
                        .types{
                                wasm::FunctionType{
                                        .parameters{
                                                wasm::ValueType{wasm::ValueType::Int32},
                                                wasm::ValueType{wasm::ValueType::Int64},
                                                wasm::ValueType{wasm::ValueType::Float32},
                                                wasm::ValueType{wasm::ValueType::Float64},
                                                wasm::ValueType{wasm::ValueType::Vector128},
                                                wasm::ValueType{wasm::ValueType::FunctionReference},
                                                wasm::ValueType{wasm::ValueType::ExternReference},
                                        },
                                },
                        },
                });
    });

    etest::test("type section, invalid value type", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Type, {1, 0x60, 0, 1, 0x10}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidTypeSection});
    });
}

void code_section_tests() {
    etest::test("code section, missing type data", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Code, {}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidCodeSection});
    });

    etest::test("code section, empty", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Code, {0})).value();
        expect_eq(module.code_section, wasm::CodeSection{});
    });

    etest::test("code section, missing data after count", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Code, {1}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidCodeSection});
    });

    etest::test("code section, missing local count", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Code, {1, 1, 1}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidCodeSection});
    });

    etest::test("code section, missing local type", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Code, {1, 1, 1, 1}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidCodeSection});
    });

    etest::test("code section, not enough data", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Code, {1, 6, 1, 1, 0x7f, 4, 4}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidCodeSection});
    });

    etest::test("code section, one entry", [] {
        auto module = wasm::Module::parse_from(make_module_bytes(SectionId::Code, {1, 6, 1, 1, 0x7f, 4, 4, 4})).value();

        wasm::CodeSection expected{.entries{
                wasm::CodeEntry{
                        .code{4, 4, 4},
                        .locals{{1, wasm::ValueType{wasm::ValueType::Int32}}},
                },
        }};
        expect_eq(module.code_section, expected);
    });

    etest::test("code section, two entries", [] {
        auto module = wasm::Module::parse_from(
                make_module_bytes(SectionId::Code, {2, 6, 1, 1, 0x7f, 4, 4, 4, 9, 2, 5, 0x7e, 6, 0x7d, 7, 8, 9, 10}))
                              .value();

        wasm::CodeSection expected{.entries{
                wasm::CodeEntry{
                        .code{4, 4, 4},
                        .locals{{1, wasm::ValueType{wasm::ValueType::Int32}}},
                },
                wasm::CodeEntry{
                        .code{7, 8, 9, 10},
                        .locals{{5, wasm::ValueType{wasm::ValueType::Int64}},
                                {6, wasm::ValueType{wasm::ValueType::Float32}}},
                },
        }};
        expect_eq(module.code_section, expected);
    });
}

} // namespace

int main() {
    etest::test("invalid magic", [] {
        auto wasm_bytes = std::stringstream{"hello"};
        expect_eq(wasm::Module::parse_from(wasm_bytes), tl::unexpected{wasm::ModuleParseError::InvalidMagic});
    });

    etest::test("unsupported version", [] {
        auto wasm_bytes = std::stringstream{"\0asm\2\0\0\0"s};
        expect_eq(wasm::Module::parse_from(wasm_bytes), tl::unexpected{wasm::ModuleParseError::UnsupportedVersion});
    });

    // https://webassembly.github.io/spec/core/syntax/modules.html
    // Each of the vectors – and thus the entire module – may be empty
    etest::test("empty module", [] {
        auto wasm_bytes = std::stringstream{"\0asm\1\0\0\0"s};
        expect_eq(wasm::Module::parse_from(std::move(wasm_bytes)), wasm::Module{});
    });

    etest::test("invalid section id", [] {
        auto wasm_bytes = std::stringstream{"\0asm\1\0\0\0\x0d"s};
        expect_eq(wasm::Module::parse_from(std::move(wasm_bytes)),
                tl::unexpected{wasm::ModuleParseError::InvalidSectionId});
    });

    etest::test("missing size", [] {
        auto wasm_bytes = std::stringstream{"\0asm\1\0\0\0\0"s};
        expect_eq(
                wasm::Module::parse_from(std::move(wasm_bytes)), tl::unexpected{wasm::ModuleParseError::UnexpectedEof});
    });

    etest::test("invalid size", [] {
        auto wasm_bytes = std::stringstream{"\0asm\1\0\0\0\0\x80\x80\x80\x80\x80\x80"s};
        expect_eq(wasm::Module::parse_from(std::move(wasm_bytes)), tl::unexpected{wasm::ModuleParseError::InvalidSize});
    });

    etest::test("unhandled section", [] {
        expect_eq(wasm::Module::parse_from(make_module_bytes(SectionId::Custom, {})),
                tl::unexpected{wasm::ModuleParseError::UnhandledSection});
    });

    type_section_tests();
    function_section_tests();
    table_section_tests();
    memory_section_tests();
    export_section_tests();
    start_section_tests();
    code_section_tests();

    return etest::run_all_tests();
}
