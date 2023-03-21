// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "wasm/wasm.h"

#include "etest/etest.h"

#include <optional>
#include <sstream>
#include <string>
#include <utility>

using namespace std::literals;

using etest::expect_eq;

namespace {

void export_section_tests() {
    etest::test("export section, non-existent", [] {
        auto module = wasm::Module{};
        expect_eq(module.export_section(), std::nullopt);
    });

    etest::test("export section, missing export count", [] {
        auto module = wasm::Module{.sections{wasm::Section{
                .id = wasm::SectionId::Export,
                .content{},
        }}};

        expect_eq(module.export_section(), std::nullopt);
    });

    etest::test("export section, missing export after count", [] {
        auto module = wasm::Module{.sections{wasm::Section{
                .id = wasm::SectionId::Export,
                .content{1},
        }}};

        expect_eq(module.export_section(), std::nullopt);
    });

    etest::test("export section, empty", [] {
        auto module = wasm::Module{.sections{wasm::Section{
                .id = wasm::SectionId::Export,
                .content{0},
        }}};

        expect_eq(module.export_section(), wasm::ExportSection{});
    });

    etest::test("export section, one", [] {
        auto module = wasm::Module{.sections{wasm::Section{
                .id = wasm::SectionId::Export,
                .content{1, 2, 'h', 'i', static_cast<std::uint8_t>(wasm::Export::Type::Function), 5},
        }}};

        expect_eq(module.export_section(),
                wasm::ExportSection{.exports{wasm::Export{"hi", wasm::Export::Type::Function, 5}}});
    });

    etest::test("export section, two", [] {
        auto module = wasm::Module{.sections{wasm::Section{
                .id = wasm::SectionId::Export,
                .content{
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
                },
        }}};

        expect_eq(module.export_section(),
                wasm::ExportSection{.exports{
                        wasm::Export{"hi", wasm::Export::Type::Function, 5},
                        wasm::Export{"lol", wasm::Export::Type::Global, 2},
                }});
    });

    etest::test("export section, missing name", [] {
        auto module = wasm::Module{.sections{wasm::Section{.id = wasm::SectionId::Export, .content{1, 2}}}};
        expect_eq(module.export_section(), std::nullopt);
    });

    etest::test("export section, missing type", [] {
        auto module = wasm::Module{.sections{wasm::Section{.id = wasm::SectionId::Export, .content{1, 1, 'a'}}}};
        expect_eq(module.export_section(), std::nullopt);
    });

    etest::test("export section, missing index", [] {
        auto module = wasm::Module{.sections{wasm::Section{.id = wasm::SectionId::Export, .content{1, 1, 'a', 1}}}};
        expect_eq(module.export_section(), std::nullopt);
    });
}

void function_section_tests() {
    etest::test("function section, non-existent", [] {
        auto module = wasm::Module{};
        expect_eq(module.function_section(), std::nullopt);
    });

    etest::test("function section, missing data", [] {
        auto module = wasm::Module{.sections{wasm::Section{
                .id = wasm::SectionId::Function,
                .content{},
        }}};
        expect_eq(module.function_section(), std::nullopt);
    });

    etest::test("function section, empty", [] {
        auto module = wasm::Module{.sections{wasm::Section{
                .id = wasm::SectionId::Function,
                .content{0},
        }}};
        expect_eq(module.function_section(), wasm::FunctionSection{});
    });

    etest::test("function section, missing type indices after count", [] {
        auto module = wasm::Module{.sections{wasm::Section{
                .id = wasm::SectionId::Function,
                .content{1},
        }}};
        expect_eq(module.function_section(), std::nullopt);
    });

    etest::test("function section, good one", [] {
        auto module = wasm::Module{.sections{wasm::Section{
                .id = wasm::SectionId::Function,
                .content{2, 9, 13},
        }}};
        expect_eq(module.function_section(), wasm::FunctionSection{.type_indices{9, 13}});
    });
}

void type_section_tests() {
    etest::test("type section, non-existent", [] {
        auto module = wasm::Module{};
        expect_eq(module.type_section(), std::nullopt);
    });

    etest::test("type section, missing type data", [] {
        auto module = wasm::Module{.sections{wasm::Section{
                .id = wasm::SectionId::Type,
                .content{},
        }}};

        expect_eq(module.type_section(), std::nullopt);
    });

    etest::test("type section, empty", [] {
        auto module = wasm::Module{.sections{wasm::Section{
                .id = wasm::SectionId::Type,
                .content{0},
        }}};

        expect_eq(module.type_section(), wasm::TypeSection{});
    });

    etest::test("type section, missing type after count", [] {
        auto module = wasm::Module{.sections{wasm::Section{
                .id = wasm::SectionId::Type,
                .content{1},
        }}};

        expect_eq(module.type_section(), std::nullopt);
    });

    etest::test("type section, bad magic in function type", [] {
        auto module = wasm::Module{.sections{wasm::Section{
                .id = wasm::SectionId::Type,
                .content{1, 0x59},
        }}};

        expect_eq(module.type_section(), std::nullopt);
    });

    etest::test("type section, one type with no parameters and no results", [] {
        auto module = wasm::Module{.sections{wasm::Section{
                .id = wasm::SectionId::Type,
                .content{1, 0x60, 0, 0},
        }}};

        expect_eq(module.type_section(),
                wasm::TypeSection{
                        .types{wasm::FunctionType{}},
                });
    });

    etest::test("type section, eof in parameter parsing", [] {
        auto module = wasm::Module{.sections{wasm::Section{
                .id = wasm::SectionId::Type,
                .content{1, 0x60, 1},
        }}};

        expect_eq(module.type_section(), std::nullopt);
    });

    etest::test("type section, eof in result parsing", [] {
        auto module = wasm::Module{.sections{wasm::Section{
                .id = wasm::SectionId::Type,
                .content{1, 0x60, 0, 1},
        }}};

        expect_eq(module.type_section(), std::nullopt);
    });

    etest::test("type section, two types", [] {
        constexpr std::uint8_t kInt32Byte = 0x7f;
        constexpr std::uint8_t kFloat64Byte = 0x7c;
        auto module = wasm::Module{.sections{wasm::Section{
                .id = wasm::SectionId::Type,
                .content{2, 0x60, 0, 1, kInt32Byte, 0x60, 2, kInt32Byte, kInt32Byte, 1, kFloat64Byte},
        }}};

        expect_eq(module.type_section(),
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
        auto module = wasm::Module{.sections{wasm::Section{
                .id = wasm::SectionId::Type,
                .content{1, 0x60, 7, 0x7f, 0x7e, 0x7d, 0x7c, 0x7b, 0x70, 0x6f, 0},
        }}};

        expect_eq(module.type_section(),
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
        auto module = wasm::Module{.sections{wasm::Section{
                .id = wasm::SectionId::Type,
                .content{1, 0x60, 0, 1, 0x10},
        }}};

        expect_eq(module.type_section(), std::nullopt);
    });
}

void code_section_tests() {
    etest::test("code section, non-existent", [] {
        auto module = wasm::Module{};
        expect_eq(module.code_section(), std::nullopt);
    });

    etest::test("code section, missing type data", [] {
        auto module = wasm::Module{.sections{wasm::Section{
                .id = wasm::SectionId::Code,
                .content{},
        }}};

        expect_eq(module.code_section(), std::nullopt);
    });

    etest::test("code section, empty", [] {
        auto module = wasm::Module{.sections{wasm::Section{
                .id = wasm::SectionId::Code,
                .content{0},
        }}};

        expect_eq(module.code_section(), wasm::CodeSection{});
    });

    etest::test("code section, missing data after count", [] {
        auto module = wasm::Module{.sections{wasm::Section{
                .id = wasm::SectionId::Code,
                .content{1},
        }}};

        expect_eq(module.code_section(), std::nullopt);
    });

    etest::test("code section, missing local count", [] {
        auto module = wasm::Module{.sections{wasm::Section{
                .id = wasm::SectionId::Code,
                .content{1, 1, 1},
        }}};

        expect_eq(module.code_section(), std::nullopt);
    });

    etest::test("code section, missing local type", [] {
        auto module = wasm::Module{.sections{wasm::Section{
                .id = wasm::SectionId::Code,
                .content{1, 1, 1, 1},
        }}};

        expect_eq(module.code_section(), std::nullopt);
    });

    etest::test("code section, not enough data", [] {
        auto module = wasm::Module{.sections{wasm::Section{
                .id = wasm::SectionId::Code,
                .content{1, 6, 1, 1, 0x7f, 4, 4},
        }}};

        expect_eq(module.code_section(), std::nullopt);
    });

    etest::test("code section, one entry", [] {
        auto module = wasm::Module{.sections{wasm::Section{
                .id = wasm::SectionId::Code,
                .content{1, 6, 1, 1, 0x7f, 4, 4, 4},
        }}};

        wasm::CodeSection expected{.entries{
                wasm::CodeEntry{
                        .code{4, 4, 4},
                        .locals{{1, wasm::ValueType{wasm::ValueType::Int32}}},
                },
        }};
        expect_eq(module.code_section(), expected);
    });

    etest::test("code section, two entries", [] {
        auto module = wasm::Module{.sections{wasm::Section{
                .id = wasm::SectionId::Code,
                .content{2, 6, 1, 1, 0x7f, 4, 4, 4, 9, 2, 5, 0x7e, 6, 0x7d, 7, 8, 9, 10},
        }}};

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
        expect_eq(module.code_section(), expected);
    });
}

} // namespace

int main() {
    etest::test("invalid magic", [] {
        auto wasm_bytes = std::stringstream{"hello"};
        expect_eq(wasm::Module::parse_from(wasm_bytes), tl::unexpected{wasm::ParseError::InvalidMagic});
    });

    etest::test("unsupported version", [] {
        auto wasm_bytes = std::stringstream{"\0asm\2\0\0\0"s};
        expect_eq(wasm::Module::parse_from(wasm_bytes), tl::unexpected{wasm::ParseError::UnsupportedVersion});
    });

    // https://webassembly.github.io/spec/core/syntax/modules.html
    // Each of the vectors – and thus the entire module – may be empty
    etest::test("empty module", [] {
        auto wasm_bytes = std::stringstream{"\0asm\1\0\0\0"s};
        expect_eq(wasm::Module::parse_from(std::move(wasm_bytes)), wasm::Module{});
    });

    etest::test("invalid section id", [] {
        auto wasm_bytes = std::stringstream{"\0asm\1\0\0\0\x0d"s};
        expect_eq(wasm::Module::parse_from(std::move(wasm_bytes)), tl::unexpected{wasm::ParseError::InvalidSectionId});
    });

    etest::test("missing size", [] {
        auto wasm_bytes = std::stringstream{"\0asm\1\0\0\0\0"s};
        expect_eq(wasm::Module::parse_from(std::move(wasm_bytes)), tl::unexpected{wasm::ParseError::Unknown});
    });

    etest::test("missing content", [] {
        auto wasm_bytes = std::stringstream{"\0asm\1\0\0\0\0\4"s};
        expect_eq(wasm::Module::parse_from(std::move(wasm_bytes)), tl::unexpected{wasm::ParseError::UnexpectedEof});
    });

    etest::test("not enough content", [] {
        auto wasm_bytes = std::stringstream{"\0asm\1\0\0\0\0\4\0\0\0"s};
        expect_eq(wasm::Module::parse_from(std::move(wasm_bytes)), tl::unexpected{wasm::ParseError::UnexpectedEof});
    });

    etest::test("one valid section", [] {
        auto wasm_bytes = std::stringstream{"\0asm\1\0\0\0\x0b\4\1\2\3\4"s};
        expect_eq(wasm::Module::parse_from(std::move(wasm_bytes)),
                wasm::Module{
                        .sections{
                                wasm::Section{
                                        .id = wasm::SectionId::Data,
                                        .content{1, 2, 3, 4},
                                },
                        },
                });
    });

    etest::test("two valid sections", [] {
        auto wasm_bytes = std::stringstream{"\0asm\1\0\0\0\x09\4\1\2\3\4\x0a\2\5\6"s};
        expect_eq(wasm::Module::parse_from(std::move(wasm_bytes)),
                wasm::Module{
                        .sections{
                                wasm::Section{
                                        .id = wasm::SectionId::Element,
                                        .content{1, 2, 3, 4},
                                },
                                wasm::Section{
                                        .id = wasm::SectionId::Code,
                                        .content{5, 6},
                                },
                        },
                });
    });

    type_section_tests();
    function_section_tests();
    export_section_tests();
    code_section_tests();

    return etest::run_all_tests();
}
