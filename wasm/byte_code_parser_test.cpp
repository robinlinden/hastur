// SPDX-FileCopyrightText: 2023-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "wasm/byte_code_parser.h"

#include "wasm/instructions.h"
#include "wasm/types.h"
#include "wasm/wasm.h"

#include "etest/etest2.h"

#include <tl/expected.hpp>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace std::literals;

using wasm::ByteCodeParser;

namespace {

// https://webassembly.github.io/spec/core/binary/modules.html#sections
enum class SectionId : std::uint8_t {
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

void parse_error_to_string_tests(etest::Suite &s) {
    using wasm::ModuleParseError;
    s.add_test("to_string(ModuleParseError)", [](etest::IActions &a) {
        a.expect_eq(wasm::to_string(ModuleParseError::UnexpectedEof), "Unexpected end of file");
        a.expect_eq(wasm::to_string(ModuleParseError::InvalidMagic), "Invalid magic number");
        a.expect_eq(wasm::to_string(ModuleParseError::UnsupportedVersion), "Unsupported version");
        a.expect_eq(wasm::to_string(ModuleParseError::InvalidSectionId), "Invalid section id");
        a.expect_eq(wasm::to_string(ModuleParseError::InvalidSize), "Invalid section size");
        a.expect_eq(wasm::to_string(ModuleParseError::InvalidCustomSection), "Invalid custom section");
        a.expect_eq(wasm::to_string(ModuleParseError::InvalidTypeSection), "Invalid type section");
        a.expect_eq(wasm::to_string(ModuleParseError::InvalidImportSection), "Invalid import section");
        a.expect_eq(wasm::to_string(ModuleParseError::InvalidFunctionSection), "Invalid function section");
        a.expect_eq(wasm::to_string(ModuleParseError::InvalidTableSection), "Invalid table section");
        a.expect_eq(wasm::to_string(ModuleParseError::InvalidMemorySection), "Invalid memory section");
        a.expect_eq(wasm::to_string(ModuleParseError::InvalidGlobalSection), "Invalid global section");
        a.expect_eq(wasm::to_string(ModuleParseError::InvalidExportSection), "Invalid export section");
        a.expect_eq(wasm::to_string(ModuleParseError::InvalidStartSection), "Invalid start section");
        a.expect_eq(wasm::to_string(ModuleParseError::InvalidCodeSection), "Invalid code section");
        a.expect_eq(wasm::to_string(ModuleParseError::InvalidDataSection), "Invalid data section");
        a.expect_eq(wasm::to_string(ModuleParseError::InvalidDataCountSection), "Invalid data count section");
        a.expect_eq(wasm::to_string(ModuleParseError::UnhandledSection), "Unhandled section");

        auto last_error_value = static_cast<int>(ModuleParseError::UnhandledSection);
        // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
        a.expect_eq(wasm::to_string(static_cast<ModuleParseError>(last_error_value + 1)), "Unknown error");
    });
}

void custom_section_tests(etest::Suite &s) {
    s.add_test("custom section", [](etest::IActions &a) {
        std::vector<std::uint8_t> content{2, 'h', 'i', 1, 2, 3};
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Custom, content)).value();
        a.expect_eq(module.custom_sections.at(0),
                wasm::CustomSection{
                        .name = "hi",
                        .data = {1, 2, 3},
                });
    });

    s.add_test("custom section, eof in name", [](etest::IActions &a) {
        std::vector<std::uint8_t> content{2, 'h'};
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Custom, content));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidCustomSection});
    });

    s.add_test("custom section, eof in data", [](etest::IActions &a) {
        std::stringstream wasm_bytes;
        wasm_bytes << "\0asm\1\0\0\0"sv;
        wasm_bytes << static_cast<std::uint8_t>(SectionId::Custom);
        wasm_bytes << static_cast<std::uint8_t>(100);
        wasm_bytes << "\2hi";
        wasm_bytes << "123";
        auto module = ByteCodeParser::parse_module(wasm_bytes);
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidCustomSection});
    });

    s.add_test("custom section, bad size (negative after name)", [](etest::IActions &a) {
        auto wasm_bytes = std::stringstream{"\0asm\1\0\0\0\0\0\0\0\0"s};
        a.expect_eq(ByteCodeParser::parse_module(std::move(wasm_bytes)),
                tl::unexpected{wasm::ModuleParseError::InvalidCustomSection});
    });

    s.add_test("custom section, bad size (too large after name)", [](etest::IActions &a) {
        auto wasm_bytes = std::stringstream{"\0asm\1\0\0\0\0\xe5\x85\x26\0\0\0\0"s};
        a.expect_eq(ByteCodeParser::parse_module(std::move(wasm_bytes)),
                tl::unexpected{wasm::ModuleParseError::InvalidCustomSection});
    });
}

void export_section_tests(etest::Suite &s) {
    s.add_test("export section, missing export count", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Export, {}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidExportSection});
    });

    s.add_test("export section, missing export after count", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Export, {1}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidExportSection});
    });

    s.add_test("export section, empty", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Export, {0})).value();
        a.expect_eq(module.export_section, wasm::ExportSection{});
    });

    s.add_test("export section, too (624485) many exports", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Export, {0xe5, 0x8e, 0x26}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidExportSection});
    });

    s.add_test("export section, name too (624485 byte) long", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Export, {1, 0xe5, 0x8e, 0x26}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidExportSection});
    });

    s.add_test("export section, one", [](etest::IActions &a) {
        std::vector<std::uint8_t> content{1, 2, 'h', 'i', static_cast<std::uint8_t>(wasm::Export::Type::Function), 5};

        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Export, content)).value();
        a.expect_eq(module.export_section,
                wasm::ExportSection{.exports{wasm::Export{"hi", wasm::Export::Type::Function, 5}}});
    });

    s.add_test("export section, two", [](etest::IActions &a) {
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
        a.expect_eq(module.export_section,
                wasm::ExportSection{.exports{
                        wasm::Export{"hi", wasm::Export::Type::Function, 5},
                        wasm::Export{"lol", wasm::Export::Type::Global, 2},
                }});
    });

    s.add_test("export section, extreme string", [](etest::IActions &a) {
        std::vector<std::uint8_t> content{1, 2, '~', '\0', static_cast<std::uint8_t>(wasm::Export::Type::Function), 5};

        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Export, content)).value();
        a.expect_eq(module.export_section,
                wasm::ExportSection{.exports{wasm::Export{"~\0"s, wasm::Export::Type::Function, 5}}});
    });

    s.add_test("export section, missing name", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Export, {1, 2}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidExportSection});
    });

    s.add_test("export section, missing type", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Export, {1, 1, 'a'}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidExportSection});
    });

    s.add_test("export section, missing index", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Export, {1, 1, 'a', 1}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidExportSection});
    });
}

void start_section_tests(etest::Suite &s) {
    s.add_test("start section, missing start", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Start, {}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidStartSection});
    });

    s.add_test("start section, excellent", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Start, {42})).value();
        a.expect_eq(module.start_section, wasm::StartSection{.start = 42});
    });
}

void function_section_tests(etest::Suite &s) {
    s.add_test("function section, missing data", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Function, {}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidFunctionSection});
    });

    s.add_test("function section, empty", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Function, {0})).value();
        a.expect_eq(module.function_section, wasm::FunctionSection{});
    });

    s.add_test("function section, missing type indices after count", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Function, {1}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidFunctionSection});
    });

    s.add_test("function section, good one", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Function, {2, 9, 13})).value();
        a.expect_eq(module.function_section, wasm::FunctionSection{.type_indices{9, 13}});
    });
}

void table_section_tests(etest::Suite &s) {
    s.add_test("table section, missing data", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Table, {}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidTableSection});
    });

    s.add_test("table section, empty", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Table, {0})).value();
        a.expect_eq(module.table_section, wasm::TableSection{});
    });

    s.add_test("table section, no element type", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Table, {1}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidTableSection});
    });

    s.add_test("table section, invalid element type", [](etest::IActions &a) {
        constexpr std::uint8_t kInt32Type = 0x7f;
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Table, {1, kInt32Type}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidTableSection});
    });

    static constexpr std::uint8_t kFuncRefType = 0x70;
    static constexpr std::uint8_t kExtRefType = 0x6f;

    s.add_test("table section, missing limits", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Table, {1, kFuncRefType}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidTableSection});
    });

    s.add_test("table section, invalid has_max in limits", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Table, {1, kFuncRefType, 4}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidTableSection});
    });

    s.add_test("table section, missing min in limits", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Table, {1, kFuncRefType, 0}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidTableSection});
    });

    s.add_test("table section, only min", [](etest::IActions &a) {
        auto module =
                ByteCodeParser::parse_module(make_module_bytes(SectionId::Table, {1, kFuncRefType, 0, 42})).value();
        a.expect_eq(module.table_section,
                wasm::TableSection{.tables{
                        wasm::TableType{
                                wasm::ValueType::FunctionReference,
                                wasm::Limits{.min = 42},
                        },
                }});
    });

    s.add_test("table section, missing max in limits", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Table, {1, kExtRefType, 1, 42}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidTableSection});
    });

    s.add_test("table section, min and max", [](etest::IActions &a) {
        auto module =
                ByteCodeParser::parse_module(make_module_bytes(SectionId::Table, {1, kExtRefType, 1, 42, 42})).value();
        a.expect_eq(module.table_section,
                wasm::TableSection{.tables{
                        wasm::TableType{
                                wasm::ValueType::ExternReference,
                                wasm::Limits{.min = 42, .max = 42},
                        },
                }});
    });
}

void memory_section_tests(etest::Suite &s) {
    s.add_test("memory section, missing data", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Memory, {}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidMemorySection});
    });

    s.add_test("memory section, empty", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Memory, {0})).value();
        a.expect_eq(module.memory_section, wasm::MemorySection{});
    });

    s.add_test("memory section, missing limits", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Memory, {1}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidMemorySection});
    });

    s.add_test("memory section, invalid has_max in limits", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Memory, {1, 4}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidMemorySection});
    });

    s.add_test("memory section, missing min in limits", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Memory, {1, 0}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidMemorySection});
    });

    s.add_test("memory section, only min", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Memory, {1, 0, 42})).value();
        a.expect_eq(module.memory_section,
                wasm::MemorySection{.memories{
                        wasm::MemType{.min = 42},
                }});
    });

    s.add_test("memory section, missing max in limits", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Memory, {1, 1, 42}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidMemorySection});
    });

    s.add_test("memory section, min and max", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Memory, {1, 1, 42, 42})).value();
        a.expect_eq(module.memory_section,
                wasm::MemorySection{.memories{
                        wasm::Limits{.min = 42, .max = 42},
                }});
    });

    s.add_test("memory section, two memories", [](etest::IActions &a) {
        auto module =
                ByteCodeParser::parse_module(make_module_bytes(SectionId::Memory, {2, 1, 4, 51, 1, 19, 84})).value();
        a.expect_eq(module.memory_section,
                wasm::MemorySection{.memories{
                        wasm::Limits{.min = 4, .max = 51},
                        wasm::Limits{.min = 19, .max = 84},
                }});
    });
}

void global_section_tests(etest::Suite &s) {
    s.add_test("global section, missing data", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Global, {}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidGlobalSection});
    });

    s.add_test("global section, empty", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Global, {0})).value();
        a.expect_eq(module.global_section, wasm::GlobalSection{});
    });

    s.add_test("global section, missing global after count", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Global, {1}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidGlobalSection});
    });

    s.add_test("global section, missing globaltype valuetype", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Global, {1}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidGlobalSection});
    });

    s.add_test("global section, missing globaltype mutability", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Global, {1, 0x7f}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidGlobalSection});
    });

    s.add_test("global section, invalid globaltype mutability", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Global, {1, 0x7f, 2}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidGlobalSection});
    });

    s.add_test("global section, missing init", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Global, {1, 0x7f, 0}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidGlobalSection});
    });

    s.add_test("global section, const i32 42", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Global, {1, 0x7f, 0, 0x41, 42, 0x0b}))
                              .value();
        a.expect_eq(module.global_section,
                wasm::GlobalSection{.globals{{
                        .type{wasm::ValueType::Int32, wasm::GlobalType::Mutability::Const},
                        .init{wasm::instructions::I32Const{42}, wasm::instructions::End{}},
                }}});
    });

    s.add_test("global section, var i32 42", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Global, {1, 0x7f, 1, 0x41, 42, 0x0b}))
                              .value();
        a.expect_eq(module.global_section,
                wasm::GlobalSection{.globals{{
                        .type{wasm::ValueType::Int32, wasm::GlobalType::Mutability::Var},
                        .init{wasm::instructions::I32Const{42}, wasm::instructions::End{}},
                }}});
    });

    s.add_test("global section, multiple globals", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(
                make_module_bytes(SectionId::Global, {2, 0x7f, 1, 0x41, 42, 0x0b, 0x7f, 0, 0x41, 42, 0x0b}))
                              .value();
        a.expect_eq(module.global_section,
                wasm::GlobalSection{.globals{
                        {
                                .type{wasm::ValueType::Int32, wasm::GlobalType::Mutability::Var},
                                .init{wasm::instructions::I32Const{42}, wasm::instructions::End{}},
                        },
                        {
                                .type{wasm::ValueType::Int32, wasm::GlobalType::Mutability::Const},
                                .init{wasm::instructions::I32Const{42}, wasm::instructions::End{}},
                        },
                }});
    });
}

void type_section_tests(etest::Suite &s) {
    s.add_test("type section, missing type data", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Type, {}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidTypeSection});
    });

    s.add_test("type section, empty", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Type, {0})).value();
        a.expect_eq(module.type_section, wasm::TypeSection{});
    });

    s.add_test("type section, missing type after count", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Type, {1}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidTypeSection});
    });

    s.add_test("type section, bad magic in function type", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Type, {1, 0x59}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidTypeSection});
    });

    s.add_test("type section, one type with no parameters and no results", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Type, {1, 0x60, 0, 0})).value();
        a.expect_eq(module.type_section, wasm::TypeSection{.types{wasm::FunctionType{}}});
    });

    s.add_test("type section, eof in parameter parsing", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Type, {1, 0x60, 1}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidTypeSection});
    });

    s.add_test("type section, eof in result parsing", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Type, {1, 0x60, 0, 1}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidTypeSection});
    });

    s.add_test("type section, two types", [](etest::IActions &a) {
        constexpr std::uint8_t kInt32Byte = 0x7f;
        constexpr std::uint8_t kFloat64Byte = 0x7c;
        auto module = ByteCodeParser::parse_module(
                make_module_bytes(
                        SectionId::Type, {2, 0x60, 0, 1, kInt32Byte, 0x60, 2, kInt32Byte, kInt32Byte, 1, kFloat64Byte}))
                              .value();

        a.expect_eq(module.type_section,
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

    s.add_test("type section, all types", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(
                make_module_bytes(SectionId::Type, {1, 0x60, 7, 0x7f, 0x7e, 0x7d, 0x7c, 0x7b, 0x70, 0x6f, 0}))
                              .value();

        a.expect_eq(module.type_section,
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

    s.add_test("type section, invalid value type", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Type, {1, 0x60, 0, 1, 0x10}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidTypeSection});
    });
}

void import_section_tests(etest::Suite &s) {
    s.add_test("import section, missing import count", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Import, {}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidImportSection});
    });

    s.add_test("import section, empty", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Import, {0})).value();
        a.expect_eq(module.import_section, wasm::ImportSection{});
    });

    s.add_test("import section, missing module name", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Import, {1}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidImportSection});
    });

    s.add_test("import section, missing field name", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Import, {1, 1, 'a'}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidImportSection});
    });

    s.add_test("import section, missing import type", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Import, {1, 1, 'a', 1, 'b'}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidImportSection});
    });

    s.add_test("import section, invalid import type", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Import, {1, 1, 'a', 1, 'b', 5}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidImportSection});
    });

    s.add_test("import section, func", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Import, {1, 1, 'a', 1, 'b', 0, 42}));
        a.expect_eq(module.value().import_section,
                wasm::ImportSection{.imports{wasm::Import{"a", "b", wasm::TypeIdx{42}}}});
    });

    s.add_test("import section, table", [](etest::IActions &a) {
        auto module =
                ByteCodeParser::parse_module(make_module_bytes(SectionId::Import, {1, 1, 'a', 1, 'b', 1, 0x70, 0, 42}));
        a.expect_eq(module.value().import_section,
                wasm::ImportSection{.imports{
                        wasm::Import{"a", "b", wasm::TableType{wasm::ValueType::FunctionReference, {42}}},
                }});
    });

    s.add_test("import section, mem", [](etest::IActions &a) {
        auto module =
                ByteCodeParser::parse_module(make_module_bytes(SectionId::Import, {1, 1, 'a', 1, 'b', 2, 1, 12, 13}));
        a.expect_eq(module.value().import_section,
                wasm::ImportSection{.imports{
                        wasm::Import{"a", "b", wasm::MemType{.min = 12, .max = 13}},
                }});
    });

    s.add_test("import section, global", [](etest::IActions &a) {
        auto module =
                ByteCodeParser::parse_module(make_module_bytes(SectionId::Import, {1, 1, 'a', 1, 'b', 3, 0x7f, 0}));
        a.expect_eq(module.value().import_section,
                wasm::ImportSection{.imports{wasm::Import{
                        "a",
                        "b",
                        wasm::GlobalType{wasm::ValueType::Int32, wasm::GlobalType::Mutability::Const},
                }}});
    });
}

void code_section_tests(etest::Suite &s) {
    s.add_test("code section, missing type data", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Code, {}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidCodeSection});
    });

    s.add_test("code section, empty", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Code, {0})).value();
        a.expect_eq(module.code_section, wasm::CodeSection{});
    });

    s.add_test("code section, missing data after count", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Code, {1}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidCodeSection});
    });

    s.add_test("code section, missing local count", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Code, {1, 1, 1}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidCodeSection});
    });

    s.add_test("code section, missing local type", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Code, {1, 1, 1, 1}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidCodeSection});
    });

    s.add_test("code section, not enough data", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Code, {1, 6, 1, 1, 0x7f, 4, 4}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidCodeSection});
    });

    s.add_test("code section, one entry", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(
                make_module_bytes(SectionId::Code, {1, 6, 1, 1, 0x7f, 0x41, 0b11, 0x69, 0x0b}))
                              .value();

        wasm::CodeSection expected{.entries{
                wasm::CodeEntry{
                        .code{wasm::instructions::I32Const{0b11},
                                wasm::instructions::I32PopulationCount{},
                                wasm::instructions::End{}},
                        .locals{{1, wasm::ValueType::Int32}},
                },
        }};
        a.expect_eq(module.code_section, expected);
    });

    s.add_test("code section, two entries", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(
                make_module_bytes(SectionId::Code, {2, 6, 1, 1, 0x7f, 0x41, 42, 0x0b, 9, 2, 5, 0x7e, 6, 0x7d, 0x0b}))
                              .value();

        wasm::CodeSection expected{.entries{
                wasm::CodeEntry{
                        .code{wasm::instructions::I32Const{42}, wasm::instructions::End{}},
                        .locals{{1, wasm::ValueType::Int32}},
                },
                wasm::CodeEntry{
                        .code{wasm::instructions::End{}},
                        .locals{{5, wasm::ValueType::Int64}, {6, wasm::ValueType::Float32}},
                },
        }};
        a.expect_eq(module.code_section, expected);
    });

    s.add_test("code section, unhandled opcode", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Code, {1, 6, 1, 1, 0x7f, 0xff, 0x0b}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidCodeSection});
    });
}

void data_tests(etest::Suite &s) {
    using wasm::DataSection;

    s.add_test("data section, passive data, everything's fine", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Data, {1, 1, 3, 1, 2, 3})).value();
        a.expect_eq(module.data_section,
                DataSection{.data{DataSection::PassiveData{{std::byte{1}, std::byte{2}, std::byte{3}}}}});
    });

    s.add_test("data section, passive data, 2 datas", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Data, {2, 1, 1, 37, 1, 1, 42})).value();
        a.expect_eq(module.data_section,
                DataSection{
                        .data{DataSection::PassiveData{{std::byte{37}}}, DataSection::PassiveData{{std::byte{42}}}}});
    });

    auto active_data_bytes = std::vector<std::uint8_t>({
            1, // section contains 1 data
            0, // active data tag
            0x41, // i32_const 42, end
            0x2a,
            0x0b,
            3, // vec{1, 2, 3}
            1,
            2,
            3,
    });

    s.add_test("data section, active data, everything's fine", [active_data_bytes](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Data, active_data_bytes)).value();
        a.expect_eq(module.data_section,
                DataSection{.data{DataSection::ActiveData{
                        .offset{wasm::instructions::I32Const{42}, wasm::instructions::End{}},
                        .data{std::byte{1}, std::byte{2}, std::byte{3}},
                }}});
    });

    s.add_test("data section, active data w/ memidx", [active_data_bytes](etest::IActions &a) mutable {
        active_data_bytes[1] = 2; // active data w/ memory index tag
        active_data_bytes.insert(active_data_bytes.begin() + 2, 13); // memory index 13
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Data, active_data_bytes)).value();
        a.expect_eq(module.data_section,
                DataSection{.data{DataSection::ActiveData{
                        .memory_idx = 13,
                        .offset{wasm::instructions::I32Const{42}, wasm::instructions::End{}},
                        .data{std::byte{1}, std::byte{2}, std::byte{3}},
                }}});
    });

    s.add_test("data section, active data w/ memidx, invalid index", [active_data_bytes](etest::IActions &a) mutable {
        active_data_bytes[1] = 2; // active data w/ memory index tag
        active_data_bytes.resize(2); // Remove everything after the tag.
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Data, active_data_bytes));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidDataSection});
    });

    s.add_test("data section, active data, bad offset", [active_data_bytes](etest::IActions &a) mutable {
        active_data_bytes.resize(4); // Remove everything after i32_const 42.
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Data, active_data_bytes));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidDataSection});
    });

    s.add_test("data section, active data, bad init", [active_data_bytes](etest::IActions &a) mutable {
        active_data_bytes.resize(6); // Remove everything after the init size.
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Data, active_data_bytes));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidDataSection});
    });

    s.add_test("data section, passive data, eof", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Data, {1, 1, 3, 1, 2}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidDataSection});
    });

    s.add_test("data section, unhandled type", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Data, {1, 5}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidDataSection});
    });

    s.add_test("data section, missing type", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Data, {1}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidDataSection});
    });

    s.add_test("data section, empty", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::Data, {0})).value();
        a.expect_eq(module.data_section, DataSection{});
    });
}

void data_count_tests(etest::Suite &s) {
    s.add_test("data count section, 42", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::DataCount, {42})).value();
        a.expect_eq(module.data_count_section, wasm::DataCountSection{.count = 42});
    });

    s.add_test("data count section, bad count", [](etest::IActions &a) {
        auto module = ByteCodeParser::parse_module(make_module_bytes(SectionId::DataCount, {0x80}));
        a.expect_eq(module, tl::unexpected{wasm::ModuleParseError::InvalidDataCountSection});
    });
}

} // namespace

int main() {
    etest::Suite s{};

    s.add_test("invalid magic", [](etest::IActions &a) {
        auto wasm_bytes = std::stringstream{"hello"};
        a.expect_eq(ByteCodeParser::parse_module(wasm_bytes), tl::unexpected{wasm::ModuleParseError::InvalidMagic});
    });

    s.add_test("unsupported version", [](etest::IActions &a) {
        auto wasm_bytes = std::stringstream{"\0asm\2\0\0\0"s};
        a.expect_eq(
                ByteCodeParser::parse_module(wasm_bytes), tl::unexpected{wasm::ModuleParseError::UnsupportedVersion});
    });

    // https://webassembly.github.io/spec/core/syntax/modules.html
    // Each of the vectors – and thus the entire module – may be empty
    s.add_test("empty module", [](etest::IActions &a) {
        auto wasm_bytes = std::stringstream{"\0asm\1\0\0\0"s};
        a.expect_eq(ByteCodeParser::parse_module(std::move(wasm_bytes)), wasm::Module{});
    });

    s.add_test("invalid section id", [](etest::IActions &a) {
        auto wasm_bytes = std::stringstream{"\0asm\1\0\0\0\x0d"s};
        a.expect_eq(ByteCodeParser::parse_module(std::move(wasm_bytes)),
                tl::unexpected{wasm::ModuleParseError::InvalidSectionId});
    });

    s.add_test("missing size", [](etest::IActions &a) {
        auto wasm_bytes = std::stringstream{"\0asm\1\0\0\0\0"s};
        a.expect_eq(ByteCodeParser::parse_module(std::move(wasm_bytes)),
                tl::unexpected{wasm::ModuleParseError::UnexpectedEof});
    });

    s.add_test("invalid size", [](etest::IActions &a) {
        auto wasm_bytes = std::stringstream{"\0asm\1\0\0\0\0\x80\x80\x80\x80\x80\x80"s};
        a.expect_eq(ByteCodeParser::parse_module(std::move(wasm_bytes)),
                tl::unexpected{wasm::ModuleParseError::InvalidSize});
    });

    s.add_test("unhandled section", [](etest::IActions &a) {
        a.expect_eq(ByteCodeParser::parse_module(make_module_bytes(SectionId::Element, {})),
                tl::unexpected{wasm::ModuleParseError::UnhandledSection});
    });

    parse_error_to_string_tests(s);
    custom_section_tests(s);
    type_section_tests(s);
    import_section_tests(s);
    function_section_tests(s);
    table_section_tests(s);
    memory_section_tests(s);
    global_section_tests(s);
    export_section_tests(s);
    start_section_tests(s);
    code_section_tests(s);
    data_tests(s);
    data_count_tests(s);

    return s.run();
}
