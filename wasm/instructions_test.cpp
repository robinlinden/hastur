// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2024 David Zero <zero-one@zer0-one.net>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "wasm/instructions.h"

#include "wasm/byte_code_parser.h"
#include "wasm/types.h"

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
    return wasm::ByteCodeParser::parse_instructions(ss);
}
} // namespace

// https://webassembly.github.io/spec/core/binary/instructions.html
int main() {
    // NOLINTBEGIN(modernize-raw-string-literal): This is hex data, not 'A'.
    etest::Suite s{"wasm::instructions"};
    using namespace wasm::instructions;

    s.add_test("block", [](etest::IActions &a) {
        // No instructions, empty function prototype.
        a.expect_eq(parse("\x02\x40\x0b\x0b"), InsnVec{Block{.type{BlockType::Empty{}}}, End{}, End{}});
        // No instructions, function returning an f32.
        a.expect_eq(parse("\x02\x7d\x0b\x0b"), InsnVec{Block{.type{wasm::ValueType::Float32}}, End{}, End{}});
        // Return, empty function prototype.
        a.expect_eq(parse("\x02\x40\x0f\x0b\x0b"), InsnVec{Block{.type{BlockType::Empty{}}}, Return{}, End{}, End{}});

        // Unexpected eof.
        a.expect_eq(parse("\x02"), std::nullopt);
        a.expect_eq(parse("\x02\x40"), std::nullopt);
        // Unhandled block type.
        a.expect_eq(parse("\x02\x0a\x0b\x0b"), std::nullopt);
    });

    s.add_test("loop", [](etest::IActions &a) {
        // No instructions, empty function prototype.
        a.expect_eq(parse("\x03\x40\x0b\x0b"), InsnVec{Loop{.type{BlockType::Empty{}}}, End{}, End{}});
        // No instructions, function returning an f32.
        a.expect_eq(parse("\x03\x7d\x0b\x0b"), InsnVec{Loop{.type{wasm::ValueType::Float32}}, End{}, End{}});
        // Return, empty function prototype.
        a.expect_eq(parse("\x03\x40\x0f\x0b\x0b"), InsnVec{Loop{.type{BlockType::Empty{}}}, Return{}, End{}, End{}});

        // Unexpected eof.
        a.expect_eq(parse("\x03"), std::nullopt);
        a.expect_eq(parse("\x03\x40"), std::nullopt);
        // Unhandled block type.
        a.expect_eq(parse("\x03\x0a\x0b\x0b"), std::nullopt);
    });

    s.add_test("branch", [](etest::IActions &a) {
        // Valid label index.
        a.expect_eq(parse("\x0c\x09\x0b"), InsnVec{Branch{.label_idx = 0x09}, End{}});

        // Unexpected eof.
        a.expect_eq(parse("\x0c"), std::nullopt);
        // Invalid label index.
        a.expect_eq(parse("\x0c\x80\x0b"), std::nullopt);
    });

    s.add_test("branch_if", [](etest::IActions &a) {
        // Valid label index.
        a.expect_eq(parse("\x0d\x09\x0b"), InsnVec{BranchIf{.label_idx = 0x09}, End{}});

        // Unexpected eof.
        a.expect_eq(parse("\x0d"), std::nullopt);
        // Invalid label index.
        a.expect_eq(parse("\x0d\x80\x0b"), std::nullopt);
    });

    s.add_test("i32_const", [](etest::IActions &a) {
        // Valid value.
        a.expect_eq(parse("\x41\x20\x0b"), InsnVec{I32Const{.value = 0x20}, End{}});

        // Unexpected eof.
        a.expect_eq(parse("\x41"), std::nullopt);
        // Invalid value.
        a.expect_eq(parse("\x41\x80\x0b"), std::nullopt);
    });

    s.add_test("i32_eqz", [](etest::IActions &a) {
        a.expect_eq(parse("\x45\x0b"), InsnVec{I32EqualZero{}, End{}}); //
    });

    s.add_test("i32_eq", [](etest::IActions &a) {
        a.expect_eq(parse("\x46\x0b"), InsnVec{I32Equal{}, End{}}); //
    });

    s.add_test("i32_ne", [](etest::IActions &a) {
        a.expect_eq(parse("\x47\x0b"), InsnVec{I32NotEqual{}, End{}}); //
    });

    s.add_test("i32_less_than_signed", [](etest::IActions &a) {
        a.expect_eq(parse("\x48\x0b"), InsnVec{I32LessThanSigned{}, End{}}); //
    });

    s.add_test("i32_less_than_unsigned", [](etest::IActions &a) {
        a.expect_eq(parse("\x49\x0b"), InsnVec{I32LessThanUnsigned{}, End{}}); //
    });

    s.add_test("i32_greater_than_signed", [](etest::IActions &a) {
        a.expect_eq(parse("\x4a\x0b"), InsnVec{I32GreaterThanSigned{}, End{}}); //
    });

    s.add_test("i32_greater_than_unsigned", [](etest::IActions &a) {
        a.expect_eq(parse("\x4b\x0b"), InsnVec{I32GreaterThanUnsigned{}, End{}}); //
    });

    s.add_test("i32_less_than_equal_signed", [](etest::IActions &a) {
        a.expect_eq(parse("\x4c\x0b"), InsnVec{I32LessThanEqualSigned{}, End{}}); //
    });

    s.add_test("i32_less_than_equal_unsigned", [](etest::IActions &a) {
        a.expect_eq(parse("\x4d\x0b"), InsnVec{I32LessThanEqualUnsigned{}, End{}}); //
    });

    s.add_test("i32_greater_than_equal_signed", [](etest::IActions &a) {
        a.expect_eq(parse("\x4e\x0b"), InsnVec{I32GreaterThanEqualSigned{}, End{}}); //
    });

    s.add_test("i32_greater_than_equal_unsigned", [](etest::IActions &a) {
        a.expect_eq(parse("\x4f\x0b"), InsnVec{I32GreaterThanEqualUnsigned{}, End{}}); //
    });

    s.add_test("i32_count_leading_zeros", [](etest::IActions &a) {
        a.expect_eq(parse("\x67\x0b"), InsnVec{I32CountLeadingZeros{}, End{}}); //
    });

    s.add_test("i32_count_trailing_zeros", [](etest::IActions &a) {
        a.expect_eq(parse("\x68\x0b"), InsnVec{I32CountTrailingZeros{}, End{}}); //
    });

    s.add_test("i32_population_count", [](etest::IActions &a) {
        a.expect_eq(parse("\x69\x0b"), InsnVec{I32PopulationCount{}, End{}}); //
    });

    s.add_test("i32_add", [](etest::IActions &a) {
        a.expect_eq(parse("\x6a\x0b"), InsnVec{I32Add{}, End{}}); //
    });

    s.add_test("i32_subtract", [](etest::IActions &a) {
        a.expect_eq(parse("\x6b\x0b"), InsnVec{I32Subtract{}, End{}}); //
    });

    s.add_test("i32_multiply", [](etest::IActions &a) {
        a.expect_eq(parse("\x6c\x0b"), InsnVec{I32Multiply{}, End{}}); //
    });

    s.add_test("i32_divide_signed", [](etest::IActions &a) {
        a.expect_eq(parse("\x6d\x0b"), InsnVec{I32DivideSigned{}, End{}}); //
    });

    s.add_test("i32_divide_unsigned", [](etest::IActions &a) {
        a.expect_eq(parse("\x6e\x0b"), InsnVec{I32DivideUnsigned{}, End{}}); //
    });

    s.add_test("i32_remainder_signed", [](etest::IActions &a) {
        a.expect_eq(parse("\x6f\x0b"), InsnVec{I32RemainderSigned{}, End{}}); //
    });

    s.add_test("i32_remainder_unsigned", [](etest::IActions &a) {
        a.expect_eq(parse("\x70\x0b"), InsnVec{I32RemainderUnsigned{}, End{}}); //
    });

    s.add_test("i32_and", [](etest::IActions &a) {
        a.expect_eq(parse("\x71\x0b"), InsnVec{I32And{}, End{}}); //
    });

    s.add_test("i32_or", [](etest::IActions &a) {
        a.expect_eq(parse("\x72\x0b"), InsnVec{I32Or{}, End{}}); //
    });

    s.add_test("i32_exclusive_or", [](etest::IActions &a) {
        a.expect_eq(parse("\x73\x0b"), InsnVec{I32ExclusiveOr{}, End{}}); //
    });

    s.add_test("i32_shift_left", [](etest::IActions &a) {
        a.expect_eq(parse("\x74\x0b"), InsnVec{I32ShiftLeft{}, End{}}); //
    });

    s.add_test("i32_shift_right_signed", [](etest::IActions &a) {
        a.expect_eq(parse("\x75\x0b"), InsnVec{I32ShiftRightSigned{}, End{}}); //
    });

    s.add_test("i32_shift_right_unsigned", [](etest::IActions &a) {
        a.expect_eq(parse("\x76\x0b"), InsnVec{I32ShiftRightUnsigned{}, End{}}); //
    });

    s.add_test("i32_rotate_left", [](etest::IActions &a) {
        a.expect_eq(parse("\x77\x0b"), InsnVec{I32RotateLeft{}, End{}}); //
    });

    s.add_test("i32_rotate_right", [](etest::IActions &a) {
        a.expect_eq(parse("\x78\x0b"), InsnVec{I32RotateRight{}, End{}}); //
    });

    s.add_test("i32_wrap_i64", [](etest::IActions &a) {
        a.expect_eq(parse("\xa7\x0b"), InsnVec{I32WrapI64{}, End{}}); //
    });

    s.add_test("i32_truncate_f32_signed", [](etest::IActions &a) {
        a.expect_eq(parse("\xa8\x0b"), InsnVec{I32TruncateF32Signed{}, End{}}); //
    });

    s.add_test("i32_truncate_f32_unsigned", [](etest::IActions &a) {
        a.expect_eq(parse("\xa9\x0b"), InsnVec{I32TruncateF32Unsigned{}, End{}}); //
    });

    s.add_test("i32_truncate_f64_signed", [](etest::IActions &a) {
        a.expect_eq(parse("\xaa\x0b"), InsnVec{I32TruncateF64Signed{}, End{}}); //
    });

    s.add_test("i32_truncate_f64_unsigned", [](etest::IActions &a) {
        a.expect_eq(parse("\xab\x0b"), InsnVec{I32TruncateF64Unsigned{}, End{}}); //
    });

    s.add_test("i32_reinterpret_f32", [](etest::IActions &a) {
        a.expect_eq(parse("\xbc\x0b"), InsnVec{I32ReinterpretF32{}, End{}}); //
    });

    s.add_test("i32_extend8_signed", [](etest::IActions &a) {
        a.expect_eq(parse("\xc0\x0b"), InsnVec{I32Extend8Signed{}, End{}}); //
    });

    s.add_test("i32_extend16_signed", [](etest::IActions &a) {
        a.expect_eq(parse("\xc1\x0b"), InsnVec{I32Extend16Signed{}, End{}}); //
    });

    s.add_test("local_get", [](etest::IActions &a) {
        // Valid index.
        a.expect_eq(parse("\x20\x09\x0b"), InsnVec{LocalGet{.idx = 0x09}, End{}});

        // Unexpected eof.
        a.expect_eq(parse("\x20"), std::nullopt);
        // Invalid index.
        a.expect_eq(parse("\x20\x80\x0b"), std::nullopt);
    });

    s.add_test("local_set", [](etest::IActions &a) {
        // Valid index.
        a.expect_eq(parse("\x21\x09\x0b"), InsnVec{LocalSet{.idx = 0x09}, End{}});

        // Unexpected eof.
        a.expect_eq(parse("\x21"), std::nullopt);
        // Invalid index.
        a.expect_eq(parse("\x21\x80\x0b"), std::nullopt);
    });

    s.add_test("local_tee", [](etest::IActions &a) {
        // Valid index.
        a.expect_eq(parse("\x22\x09\x0b"), InsnVec{LocalTee{.idx = 0x09}, End{}});

        // Unexpected eof.
        a.expect_eq(parse("\x22"), std::nullopt);
        // Invalid index.
        a.expect_eq(parse("\x22\x80\x0b"), std::nullopt);
    });

    s.add_test("i32_load", [](etest::IActions &a) {
        // Valid memarg.
        a.expect_eq(parse("\x28\x0a\x0c\x0b"), InsnVec{I32Load{MemArg{.align = 0x0a, .offset = 0x0c}}, End{}});

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
