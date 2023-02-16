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

} // namespace

int main() {
    etest::test("invalid magic", [] {
        auto wasm_bytes = std::stringstream{"hello"};
        expect_eq(wasm::Module::parse_from(wasm_bytes), std::nullopt);
    });

    etest::test("unsupported version", [] {
        auto wasm_bytes = std::stringstream{"\0asm\2\0\0\0"s};
        expect_eq(wasm::Module::parse_from(wasm_bytes), std::nullopt);
    });

    // https://webassembly.github.io/spec/core/bikeshed/#modules
    // Each of the vectors – and thus the entire module – may be empty
    etest::test("empty module", [] {
        auto wasm_bytes = std::stringstream{"\0asm\1\0\0\0"s};
        expect_eq(wasm::Module::parse_from(std::move(wasm_bytes)), wasm::Module{});
    });

    etest::test("invalid section id", [] {
        auto wasm_bytes = std::stringstream{"\0asm\1\0\0\0\x0d"s};
        expect_eq(wasm::Module::parse_from(std::move(wasm_bytes)), std::nullopt);
    });

    etest::test("missing size", [] {
        auto wasm_bytes = std::stringstream{"\0asm\1\0\0\0\0"s};
        expect_eq(wasm::Module::parse_from(std::move(wasm_bytes)), std::nullopt);
    });

    etest::test("missing content", [] {
        auto wasm_bytes = std::stringstream{"\0asm\1\0\0\0\0\4"s};
        expect_eq(wasm::Module::parse_from(std::move(wasm_bytes)), std::nullopt);
    });

    etest::test("not enough content", [] {
        auto wasm_bytes = std::stringstream{"\0asm\1\0\0\0\0\4\0\0\0"s};
        expect_eq(wasm::Module::parse_from(std::move(wasm_bytes)), std::nullopt);
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

    export_section_tests();

    return etest::run_all_tests();
}
