// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "wasm/byte_code_parser.h"

#include "wasm/instructions.h"
#include "wasm/types.h"
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
using wasm::ByteCodeParser;

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

void parse_error_to_string_tests() {
    using wasm::ModuleParseError;
    etest::test("to_string(ModuleParseError)", [] {
        expect_eq(wasm::to_string(ModuleParseError::UnexpectedEof), "Unexpected end of file");
        expect_eq(wasm::to_string(ModuleParseError::InvalidMagic), "Invalid magic number");
        expect_eq(wasm::to_string(ModuleParseError::UnsupportedVersion), "Unsupported version");
        expect_eq(wasm::to_string(ModuleParseError::InvalidSectionId), "Invalid section id");
        expect_eq(wasm::to_string(ModuleParseError::InvalidSize), "Invalid section size");
        expect_eq(wasm::to_string(ModuleParseError::InvalidCustomSection), "Invalid custom section");
        expect_eq(wasm::to_string(ModuleParseError::InvalidTypeSection), "Invalid type section");
        expect_eq(wasm::to_string(ModuleParseError::InvalidImportSection), "Invalid import section");
        expect_eq(wasm::to_string(ModuleParseError::InvalidFunctionSection), "Invalid function section");
        expect_eq(wasm::to_string(ModuleParseError::InvalidTableSection), "Invalid table section");
        expect_eq(wasm::to_string(ModuleParseError::InvalidMemorySection), "Invalid memory section");
        expect_eq(wasm::to_string(ModuleParseError::InvalidGlobalSection), "Invalid global section");
        expect_eq(wasm::to_string(ModuleParseError::InvalidExportSection), "Invalid export section");
        expect_eq(wasm::to_string(ModuleParseError::InvalidStartSection), "Invalid start section");
        expect_eq(wasm::to_string(ModuleParseError::InvalidCodeSection), "Invalid code section");
        expect_eq(wasm::to_string(ModuleParseError::UnhandledSection), "Unhandled section");

        auto last_error_value = static_cast<int>(ModuleParseError::UnhandledSection);
        expect_eq(wasm::to_string(static_cast<ModuleParseError>(last_error_value + 1)), "Unknown error");
    });
}

void custom_section_tests() {
    etest::test("custom section", [] {
        std::vector<std::uint8_t> content{2, 'h', 'i', 1, 2, 3};
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Custom, content)).value();
        expect_eq(module.custom_sections.at(0),
                wasm::CustomSection{
                        .name = "hi",
                        .data = {1, 2, 3},
                });
    });

    etest::test("custom section, eof in name", [] {
        std::vector<std::uint8_t> content{2, 'h'};
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Custom, content));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidCustomSection});
    });

    etest::test("custom section, eof in data", [] {
        std::stringstream wasm_bytes;
        wasm_bytes << "\0asm\1\0\0\0"sv;
        wasm_bytes << static_cast<std::uint8_t>(SectionId::Custom);
        wasm_bytes << static_cast<std::uint8_t>(100);
        wasm_bytes << "\2hi";
        wasm_bytes << "123";
        auto module = ByteCodeParser::parse_module(wasm_bytes);
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidCustomSection});
    });

    etest::test("custom section, bad size (negative after name)", [] {
        auto wasm_bytes = std::stringstream{"\0asm\1\0\0\0\0\0\0\0\0"s};
        expect_eq(ByteCodeParser::parse_module(std::move(wasm_bytes)),
                tl::unexpected{wasm::ModuleParseError::InvalidCustomSection});
    });

    etest::test("custom section, bad size (too large after name)", [] {
        auto wasm_bytes = std::stringstream{"\0asm\1\0\0\0\0\xe5\x85\x26\0\0\0\0"s};
        expect_eq(ByteCodeParser::parse_module(std::move(wasm_bytes)),
                tl::unexpected{wasm::ModuleParseError::InvalidCustomSection});
    });
}

void export_section_tests() {
    etest::test("export section, missing export count", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Export, {}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidExportSection});
    });

    etest::test("export section, missing export after count", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Export, {1}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidExportSection});
    });

    etest::test("export section, empty", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Export, {0})).value();
        expect_eq(module.export_section, wasm::ExportSection{});
    });

    etest::test("export section, too (624485) many exports", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Export, {0xe5, 0x8e, 0x26}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidExportSection});
    });

    etest::test("export section, name too (624485 byte) long", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Export, {1, 0xe5, 0x8e, 0x26}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidExportSection});
    });

    etest::test("export section, one", [] {
        std::vector<std::uint8_t> content{1, 2, 'h', 'i', static_cast<std::uint8_t>(wasm::Export::Type::Function), 5};

        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Export, content)).value();
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

        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Export, content)).value();
        expect_eq(module.export_section,
                wasm::ExportSection{.exports{
                        wasm::Export{"hi", wasm::Export::Type::Function, 5},
                        wasm::Export{"lol", wasm::Export::Type::Global, 2},
                }});
    });

    etest::test("export section, extreme string", [] {
        std::vector<std::uint8_t> content{1, 2, '~', '\0', static_cast<std::uint8_t>(wasm::Export::Type::Function), 5};

        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Export, content)).value();
        expect_eq(module.export_section,
                wasm::ExportSection{.exports{wasm::Export{"~\0"s, wasm::Export::Type::Function, 5}}});
    });

    etest::test("export section, missing name", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Export, {1, 2}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidExportSection});
    });

    etest::test("export section, missing type", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Export, {1, 1, 'a'}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidExportSection});
    });

    etest::test("export section, missing index", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Export, {1, 1, 'a', 1}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidExportSection});
    });
}

void start_section_tests() {
    etest::test("start section, missing start", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Start, {}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidStartSection});
    });

    etest::test("start section, excellent", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Start, {42})).value();
        expect_eq(module.start_section, wasm::StartSection{.start = 42});
    });
}

void function_section_tests() {
    etest::test("function section, missing data", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Function, {}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidFunctionSection});
    });

    etest::test("function section, empty", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Function, {0})).value();
        expect_eq(module.function_section, wasm::FunctionSection{});
    });

    etest::test("function section, missing type indices after count", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Function, {1}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidFunctionSection});
    });

    etest::test("function section, good one", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Function, {2, 9, 13})).value();
        expect_eq(module.function_section, wasm::FunctionSection{.type_indices{9, 13}});
    });
}

void table_section_tests() {
    etest::test("table section, missing data", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Table, {}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidTableSection});
    });

    etest::test("table section, empty", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Table, {0})).value();
        expect_eq(module.table_section, wasm::TableSection{});
    });

    etest::test("table section, no element type", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Table, {1}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidTableSection});
    });

    etest::test("table section, invalid element type", [] {
        constexpr std::uint8_t kInt32Type = 0x7f;
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Table, {1, kInt32Type}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidTableSection});
    });

    static constexpr std::uint8_t kFuncRefType = 0x70;
    static constexpr std::uint8_t kExtRefType = 0x6f;

    etest::test("table section, missing limits", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Table, {1, kFuncRefType}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidTableSection});
    });

    etest::test("table section, invalid has_max in limits", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Table, {1, kFuncRefType, 4}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidTableSection});
    });

    etest::test("table section, missing min in limits", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Table, {1, kFuncRefType, 0}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidTableSection});
    });

    etest::test("table section, only min", [] {
        auto module =
                ByteCodeParser::parse_module(make_module_bytes(SectionId::Table, {1, kFuncRefType, 0, 42})).value();
        expect_eq(module.table_section,
                wasm::TableSection{.tables{
                        wasm::TableType{
                                wasm::ValueType::FunctionReference,
                                wasm::Limits{.min = 42},
                        },
                }});
    });

    etest::test("table section, missing max in limits", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Table, {1, kExtRefType, 1, 42}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidTableSection});
    });

    etest::test("table section, min and max", [] {
        auto module =
                ByteCodeParser::parse_module(make_module_bytes(SectionId::Table, {1, kExtRefType, 1, 42, 42})).value();
        expect_eq(module.table_section,
                wasm::TableSection{.tables{
                        wasm::TableType{
                                wasm::ValueType::ExternReference,
                                wasm::Limits{.min = 42, .max = 42},
                        },
                }});
    });
}

void memory_section_tests() {
    etest::test("memory section, missing data", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Memory, {}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidMemorySection});
    });

    etest::test("memory section, empty", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Memory, {0})).value();
        expect_eq(module.memory_section, wasm::MemorySection{});
    });

    etest::test("memory section, missing limits", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Memory, {1}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidMemorySection});
    });

    etest::test("memory section, invalid has_max in limits", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Memory, {1, 4}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidMemorySection});
    });

    etest::test("memory section, missing min in limits", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Memory, {1, 0}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidMemorySection});
    });

    etest::test("memory section, only min", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Memory, {1, 0, 42})).value();
        expect_eq(module.memory_section,
                wasm::MemorySection{.memories{
                        wasm::MemType{.min = 42},
                }});
    });

    etest::test("memory section, missing max in limits", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Memory, {1, 1, 42}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidMemorySection});
    });

    etest::test("memory section, min and max", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Memory, {1, 1, 42, 42})).value();
        expect_eq(module.memory_section,
                wasm::MemorySection{.memories{
                        wasm::Limits{.min = 42, .max = 42},
                }});
    });

    etest::test("memory section, two memories", [] {
        auto module =
                ByteCodeParser::parse_module(make_module_bytes(SectionId::Memory, {2, 1, 4, 51, 1, 19, 84})).value();
        expect_eq(module.memory_section,
                wasm::MemorySection{.memories{
                        wasm::Limits{.min = 4, .max = 51},
                        wasm::Limits{.min = 19, .max = 84},
                }});
    });
}

void global_section_tests() {
    etest::test("global section, missing data", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Global, {}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidGlobalSection});
    });

    etest::test("global section, empty", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Global, {0})).value();
        expect_eq(module.global_section, wasm::GlobalSection{});
    });

    etest::test("global section, missing global after count", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Global, {1}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidGlobalSection});
    });

    etest::test("global section, missing globaltype valuetype", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Global, {1}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidGlobalSection});
    });

    etest::test("global section, missing globaltype mutability", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Global, {1, 0x7f}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidGlobalSection});
    });

    etest::test("global section, invalid globaltype mutability", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Global, {1, 0x7f, 2}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidGlobalSection});
    });

    etest::test("global section, missing init", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Global, {1, 0x7f, 0}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidGlobalSection});
    });

    etest::test("global section, const i32 42", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Global, {1, 0x7f, 0, 0x41, 42, 0x0b}))
                              .value();
        expect_eq(module.global_section,
                wasm::GlobalSection{.globals{{
                        .type{wasm::ValueType::Int32, wasm::GlobalType::Mutability::Const},
                        .init{wasm::instructions::I32Const{42}},
                }}});
    });

    etest::test("global section, var i32 42", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Global, {1, 0x7f, 1, 0x41, 42, 0x0b}))
                              .value();
        expect_eq(module.global_section,
                wasm::GlobalSection{.globals{{
                        .type{wasm::ValueType::Int32, wasm::GlobalType::Mutability::Var},
                        .init{wasm::instructions::I32Const{42}},
                }}});
    });

    etest::test("global section, multiple globals", [] {
        auto module = ByteCodeParser::parse_module(
                make_module_bytes(SectionId::Global, {2, 0x7f, 1, 0x41, 42, 0x0b, 0x7f, 0, 0x41, 42, 0x0b}))
                              .value();
        expect_eq(module.global_section,
                wasm::GlobalSection{.globals{
                        {
                                .type{wasm::ValueType::Int32, wasm::GlobalType::Mutability::Var},
                                .init{wasm::instructions::I32Const{42}},
                        },
                        {
                                .type{wasm::ValueType::Int32, wasm::GlobalType::Mutability::Const},
                                .init{wasm::instructions::I32Const{42}},
                        },
                }});
    });
}

void type_section_tests() {
    etest::test("type section, missing type data", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Type, {}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidTypeSection});
    });

    etest::test("type section, empty", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Type, {0})).value();
        expect_eq(module.type_section, wasm::TypeSection{});
    });

    etest::test("type section, missing type after count", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Type, {1}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidTypeSection});
    });

    etest::test("type section, bad magic in function type", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Type, {1, 0x59}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidTypeSection});
    });

    etest::test("type section, one type with no parameters and no results", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Type, {1, 0x60, 0, 0})).value();
        expect_eq(module.type_section, wasm::TypeSection{.types{wasm::FunctionType{}}});
    });

    etest::test("type section, eof in parameter parsing", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Type, {1, 0x60, 1}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidTypeSection});
    });

    etest::test("type section, eof in result parsing", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Type, {1, 0x60, 0, 1}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidTypeSection});
    });

    etest::test("type section, two types", [] {
        constexpr std::uint8_t kInt32Byte = 0x7f;
        constexpr std::uint8_t kFloat64Byte = 0x7c;
        auto module = ByteCodeParser::parse_module(
                make_module_bytes(
                        SectionId::Type, {2, 0x60, 0, 1, kInt32Byte, 0x60, 2, kInt32Byte, kInt32Byte, 1, kFloat64Byte}))
                              .value();

        expect_eq(module.type_section,
                wasm::TypeSection{
                        .types{
                                wasm::FunctionType{.results{wasm::ValueType::Int32}},
                                wasm::FunctionType{
                                        .parameters{wasm::ValueType::Int32, wasm::ValueType::Int32},
                                        .results{wasm::ValueType::Float64},
                                },
                        },
                });
    });

    etest::test("type section, all types", [] {
        auto module = ByteCodeParser::parse_module(
                make_module_bytes(SectionId::Type, {1, 0x60, 7, 0x7f, 0x7e, 0x7d, 0x7c, 0x7b, 0x70, 0x6f, 0}))
                              .value();

        expect_eq(module.type_section,
                wasm::TypeSection{
                        .types{
                                wasm::FunctionType{
                                        .parameters{
                                                wasm::ValueType::Int32,
                                                wasm::ValueType::Int64,
                                                wasm::ValueType::Float32,
                                                wasm::ValueType::Float64,
                                                wasm::ValueType::Vector128,
                                                wasm::ValueType::FunctionReference,
                                                wasm::ValueType::ExternReference,
                                        },
                                },
                        },
                });
    });

    etest::test("type section, invalid value type", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Type, {1, 0x60, 0, 1, 0x10}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidTypeSection});
    });
}

void import_section_tests() {
    etest::test("import section, missing import count", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Import, {}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidImportSection});
    });

    etest::test("import section, empty", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Import, {0})).value();
        expect_eq(module.import_section, wasm::ImportSection{});
    });

    etest::test("import section, missing module name", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Import, {1}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidImportSection});
    });

    etest::test("import section, missing field name", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Import, {1, 1, 'a'}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidImportSection});
    });

    etest::test("import section, missing import type", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Import, {1, 1, 'a', 1, 'b'}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidImportSection});
    });

    etest::test("import section, invalid import type", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Import, {1, 1, 'a', 1, 'b', 5}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidImportSection});
    });

    etest::test("import section, func", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Import, {1, 1, 'a', 1, 'b', 0, 42}));
        expect_eq(module.value().import_section,
                wasm::ImportSection{.imports{wasm::Import{"a", "b", wasm::TypeIdx{42}}}});
    });

    etest::test("import section, table", [] {
        auto module =
                ByteCodeParser::parse_module(make_module_bytes(SectionId::Import, {1, 1, 'a', 1, 'b', 1, 0x70, 0, 42}));
        expect_eq(module.value().import_section,
                wasm::ImportSection{.imports{
                        wasm::Import{"a", "b", wasm::TableType{wasm::ValueType::FunctionReference, {42}}},
                }});
    });

    etest::test("import section, mem", [] {
        auto module =
                ByteCodeParser::parse_module(make_module_bytes(SectionId::Import, {1, 1, 'a', 1, 'b', 2, 1, 12, 13}));
        expect_eq(module.value().import_section,
                wasm::ImportSection{.imports{
                        wasm::Import{"a", "b", wasm::MemType{.min = 12, .max = 13}},
                }});
    });

    etest::test("import section, global", [] {
        auto module =
                ByteCodeParser::parse_module(make_module_bytes(SectionId::Import, {1, 1, 'a', 1, 'b', 3, 0x7f, 0}));
        expect_eq(module.value().import_section,
                wasm::ImportSection{.imports{wasm::Import{
                        "a",
                        "b",
                        wasm::GlobalType{wasm::ValueType::Int32, wasm::GlobalType::Mutability::Const},
                }}});
    });
}

void code_section_tests() {
    etest::test("code section, missing type data", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Code, {}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidCodeSection});
    });

    etest::test("code section, empty", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Code, {0})).value();
        expect_eq(module.code_section, wasm::CodeSection{});
    });

    etest::test("code section, missing data after count", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Code, {1}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidCodeSection});
    });

    etest::test("code section, missing local count", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Code, {1, 1, 1}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidCodeSection});
    });

    etest::test("code section, missing local type", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Code, {1, 1, 1, 1}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidCodeSection});
    });

    etest::test("code section, not enough data", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Code, {1, 6, 1, 1, 0x7f, 4, 4}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidCodeSection});
    });

    etest::test("code section, one entry", [] {
        auto module = ByteCodeParser::parse_module(
                make_module_bytes(SectionId::Code, {1, 6, 1, 1, 0x7f, 0x41, 0b11, 0x69, 0x0b}))
                              .value();

        wasm::CodeSection expected{.entries{
                wasm::CodeEntry{
                        .code{wasm::instructions::I32Const{0b11}, wasm::instructions::I32PopulationCount{}},
                        .locals{{1, wasm::ValueType::Int32}},
                },
        }};
        expect_eq(module.code_section, expected);
    });

    etest::test("code section, two entries", [] {
        auto module = ByteCodeParser::parse_module(
                make_module_bytes(SectionId::Code, {2, 6, 1, 1, 0x7f, 0x41, 42, 0x0b, 9, 2, 5, 0x7e, 6, 0x7d, 0x0b}))
                              .value();

        wasm::CodeSection expected{.entries{
                wasm::CodeEntry{
                        .code{wasm::instructions::I32Const{42}},
                        .locals{{1, wasm::ValueType::Int32}},
                },
                wasm::CodeEntry{
                        .code{},
                        .locals{{5, wasm::ValueType::Int64}, {6, wasm::ValueType::Float32}},
                },
        }};
        expect_eq(module.code_section, expected);
    });

    etest::test("code section, unhandled opcode", [] {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Code, {1, 6, 1, 1, 0x7f, 0xff, 0x0b}));
        expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidCodeSection});
    });
}

} // namespace

int main() {
    etest::test("invalid magic", [] {
        auto wasm_bytes = std::stringstream{"hello"};
        expect_eq(ByteCodeParser::parse_module(wasm_bytes), tl::unexpected{wasm::ModuleParseError::InvalidMagic});
    });

    etest::test("unsupported version", [] {
        auto wasm_bytes = std::stringstream{"\0asm\2\0\0\0"s};
        expect_eq(ByteCodeParser::parse_module(wasm_bytes), tl::unexpected{wasm::ModuleParseError::UnsupportedVersion});
    });

    // https://webassembly.github.io/spec/core/syntax/modules.html
    // Each of the vectors – and thus the entire module – may be empty
    etest::test("empty module", [] {
        auto wasm_bytes = std::stringstream{"\0asm\1\0\0\0"s};
        expect_eq(ByteCodeParser::parse_module(std::move(wasm_bytes)), wasm::Module{});
    });

    etest::test("invalid section id", [] {
        auto wasm_bytes = std::stringstream{"\0asm\1\0\0\0\x0d"s};
        expect_eq(ByteCodeParser::parse_module(std::move(wasm_bytes)),
                tl::unexpected{wasm::ModuleParseError::InvalidSectionId});
    });

    etest::test("missing size", [] {
        auto wasm_bytes = std::stringstream{"\0asm\1\0\0\0\0"s};
        expect_eq(ByteCodeParser::parse_module(std::move(wasm_bytes)),
                tl::unexpected{wasm::ModuleParseError::UnexpectedEof});
    });

    etest::test("invalid size", [] {
        auto wasm_bytes = std::stringstream{"\0asm\1\0\0\0\0\x80\x80\x80\x80\x80\x80"s};
        expect_eq(ByteCodeParser::parse_module(std::move(wasm_bytes)),
                tl::unexpected{wasm::ModuleParseError::InvalidSize});
    });

    etest::test("unhandled section", [] {
        expect_eq(ByteCodeParser::parse_module(make_module_bytes(SectionId::DataCount, {})),
                tl::unexpected{wasm::ModuleParseError::UnhandledSection});
    });

    parse_error_to_string_tests();
    custom_section_tests();
    type_section_tests();
    import_section_tests();
    function_section_tests();
    table_section_tests();
    memory_section_tests();
    global_section_tests();
    export_section_tests();
    start_section_tests();
    code_section_tests();

    return etest::run_all_tests();
}
