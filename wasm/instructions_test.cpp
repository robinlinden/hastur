// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "wasm/instructions.h"

#include "etest/etest2.h"

#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using InsnVec = std::vector<wasm::instructions::Instruction>;

namespace {
std::optional<InsnVec> parse(std::string s) {
    std::stringstream ss{std::move(s)};
    return wasm::instructions::parse(ss);
}
} // namespace

// https://webassembly.github.io/spec/core/binary/instructions.html
int main() {
    // NOLINTBEGIN(modernize-raw-string-literal): This is hex data, not 'A'.
    etest::Suite s{"wasm::instructions"};
    using namespace wasm::instructions;

    s.add_test("block", [](etest::IActions &a) {
        // No instructions, empty function prototype.
        a.expect_eq(parse("\x02\x40\x0b\x0b"), InsnVec{Block{.type{BlockType::Empty{}}}});
        // No instructions, function returning an f32.
        a.expect_eq(parse("\x02\x7d\x0b\x0b"), InsnVec{Block{.type{wasm::ValueType{wasm::ValueType::Kind::Float32}}}});
        // Return, empty function prototype.
        a.expect_eq(parse("\x02\x40\x0f\x0b\x0b"), InsnVec{Block{.type{BlockType::Empty{}}, .instructions{Return{}}}});

        // Unexpected eof.
        a.expect_eq(parse("\x02"), std::nullopt);
        a.expect_eq(parse("\x02\x40"), std::nullopt);
        // Unhandled block type.
        a.expect_eq(parse("\x02\x0a\x0b\x0b"), std::nullopt);
    });

    s.add_test("loop", [](etest::IActions &a) {
        // No instructions, empty function prototype.
        a.expect_eq(parse("\x03\x40\x0b\x0b"), InsnVec{Loop{.type{BlockType::Empty{}}}});
        // No instructions, function returning an f32.
        a.expect_eq(parse("\x03\x7d\x0b\x0b"), InsnVec{Loop{.type{wasm::ValueType{wasm::ValueType::Kind::Float32}}}});
        // Return, empty function prototype.
        a.expect_eq(parse("\x03\x40\x0f\x0b\x0b"), InsnVec{Loop{.type{BlockType::Empty{}}, .instructions{Return{}}}});

        // Unexpected eof.
        a.expect_eq(parse("\x03"), std::nullopt);
        a.expect_eq(parse("\x03\x40"), std::nullopt);
        // Unhandled block type.
        a.expect_eq(parse("\x03\x0a\x0b\x0b"), std::nullopt);
    });

    s.add_test("break_if", [](etest::IActions &a) {
        // Valid label index.
        a.expect_eq(parse("\x0d\x09\x0b"), InsnVec{BreakIf{.label_idx = 0x09}});

        // Unexpected eof.
        a.expect_eq(parse("\x0d"), std::nullopt);
        // Invalid label index.
        a.expect_eq(parse("\x0d\x80\x0b"), std::nullopt);
    });

    s.add_test("i32_const", [](etest::IActions &a) {
        // Valid value.
        a.expect_eq(parse("\x41\x20\x0b"), InsnVec{I32Const{.value = 0x20}});

        // Unexpected eof.
        a.expect_eq(parse("\x41"), std::nullopt);
        // Invalid value.
        a.expect_eq(parse("\x41\x80\x0b"), std::nullopt);
    });

    s.add_test("i32_less_than_signed", [](etest::IActions &a) {
        a.expect_eq(parse("\x48\x0b"), InsnVec{I32LessThanSigned{}}); //
    });

    s.add_test("i32_add", [](etest::IActions &a) {
        a.expect_eq(parse("\x6a\x0b"), InsnVec{I32Add{}}); //
    });

    s.add_test("local_get", [](etest::IActions &a) {
        // Valid index.
        a.expect_eq(parse("\x20\x09\x0b"), InsnVec{LocalGet{.idx = 0x09}});

        // Unexpected eof.
        a.expect_eq(parse("\x20"), std::nullopt);
        // Invalid index.
        a.expect_eq(parse("\x20\x80\x0b"), std::nullopt);
    });

    s.add_test("local_set", [](etest::IActions &a) {
        // Valid index.
        a.expect_eq(parse("\x21\x09\x0b"), InsnVec{LocalSet{.idx = 0x09}});

        // Unexpected eof.
        a.expect_eq(parse("\x21"), std::nullopt);
        // Invalid index.
        a.expect_eq(parse("\x21\x80\x0b"), std::nullopt);
    });

    s.add_test("local_tee", [](etest::IActions &a) {
        // Valid index.
        a.expect_eq(parse("\x22\x09\x0b"), InsnVec{LocalTee{.idx = 0x09}});

        // Unexpected eof.
        a.expect_eq(parse("\x22"), std::nullopt);
        // Invalid index.
        a.expect_eq(parse("\x22\x80\x0b"), std::nullopt);
    });

    s.add_test("i32_load", [](etest::IActions &a) {
        // Valid memarg.
        a.expect_eq(parse("\x28\x0a\x0c\x0b"), InsnVec{I32Load{MemArg{.align = 0x0a, .offset = 0x0c}}});

        // Unexpected eof.
        a.expect_eq(parse("\x28"), std::nullopt);
        a.expect_eq(parse("\x28\x0a"), std::nullopt);
        // Invalid memarg.
        a.expect_eq(parse("\x28\x80\x0a\x0b"), std::nullopt);
        a.expect_eq(parse("\x28\x0a\x80\x0b"), std::nullopt);
    });

    s.add_test("unhandled opcode", [](etest::IActions &a) {
        a.expect_eq(parse("\xff"), std::nullopt); //
    });

    // NOLINTEND(modernize-raw-string-literal)
    return s.run();
}
