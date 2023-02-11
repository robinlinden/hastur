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

    return etest::run_all_tests();
}
