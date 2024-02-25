// SPDX-FileCopyrightText: 2024 David Zero <zero-one@zer0-one.net>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "wasm/instructions.h"
#include "wasm/serialize.h"
#include "wasm/types.h"

#include "etest/etest2.h"

int main() {
    etest::Suite s{"wasm module serialization"};

    using namespace wasm::instructions;

    s.add_test("block", [](etest::IActions &a) {
        a.expect_eq(to_string(Block{.type{wasm::ValueType{wasm::ValueType::Kind::Int32}},
                            .instructions{I32Const{2}, I32Const{2}, I32Add{}}}),
                "block (result i32) \n\ti32.const 2\n\ti32.const 2\n\ti32.add\nend");
        a.expect_eq(to_string(Block{.type{wasm::TypeIdx{7}}, .instructions{I32Const{2}, I32Const{2}, I32Add{}}}),
                "block (type 7) \n\ti32.const 2\n\ti32.const 2\n\ti32.add\nend");
        a.expect_eq(to_string(Block{.type{wasm::ValueType{wasm::ValueType::Kind::Int32}},
                            .instructions{Block{.type{wasm::ValueType{wasm::ValueType::Kind::Int32}},
                                                  .instructions{I32Const{8}}},
                                    I32Const{2},
                                    I32Const{2},
                                    I32Add{}}}),
                "block (result i32) \n\tblock (result i32) \n\t\ti32.const 8\n\tend\n\ti32.const 2\n\ti32.const "
                "2\n\ti32.add\nend");
    });

    s.add_test("loop", [](etest::IActions &a) {
        a.expect_eq(to_string(Loop{.type{wasm::ValueType{wasm::ValueType::Kind::Int32}},
                            .instructions{I32Const{2}, I32Const{2}, I32Add{}}}),
                "loop (result i32) \n\ti32.const 2\n\ti32.const 2\n\ti32.add\nend");
        a.expect_eq(to_string(Loop{.type{wasm::TypeIdx{7}}, .instructions{I32Const{2}, I32Const{2}, I32Add{}}}),
                "loop (type 7) \n\ti32.const 2\n\ti32.const 2\n\ti32.add\nend");
        a.expect_eq(to_string(Loop{.type{wasm::ValueType{wasm::ValueType::Kind::Int32}},
                            .instructions{Loop{.type{wasm::ValueType{wasm::ValueType::Kind::Int32}},
                                                  .instructions{I32Const{8}}},
                                    I32Const{2},
                                    I32Const{2},
                                    I32Add{}}}),
                "loop (result i32) \n\tloop (result i32) \n\t\ti32.const 8\n\tend\n\ti32.const 2\n\ti32.const "
                "2\n\ti32.add\nend");
    });

    s.add_test("break_if", [](etest::IActions &a) { a.expect_eq(to_string(BreakIf{}), "br_if 0"); });

    s.add_test("i32_const", [](etest::IActions &a) { a.expect_eq(to_string(I32Const{}), "i32.const 0"); });

    s.add_test("i32_eqz", [](etest::IActions &a) { a.expect_eq(to_string(I32EqualZero{}), "i32.eqz"); });

    s.add_test("i32_eq", [](etest::IActions &a) { a.expect_eq(to_string(I32Equal{}), "i32.eq"); });

    s.add_test("i32_ne", [](etest::IActions &a) { a.expect_eq(to_string(I32NotEqual{}), "i32.ne"); });

    s.add_test("i32_less_than_signed",
            [](etest::IActions &a) { a.expect_eq(to_string(I32LessThanSigned{}), "i32.lt_s"); });

    s.add_test("i32_less_than_unsigned",
            [](etest::IActions &a) { a.expect_eq(to_string(I32LessThanUnsigned{}), "i32.lt_u"); });

    s.add_test("i32_greater_than_signed",
            [](etest::IActions &a) { a.expect_eq(to_string(I32GreaterThanSigned{}), "i32.gt_s"); });

    s.add_test("i32_greater_than_unsigned",
            [](etest::IActions &a) { a.expect_eq(to_string(I32GreaterThanUnsigned{}), "i32.gt_u"); });

    s.add_test("i32_less_than_equal_signed",
            [](etest::IActions &a) { a.expect_eq(to_string(I32LessThanEqualSigned{}), "i32.le_s"); });

    s.add_test("i32_less_than_equal_unsigned",
            [](etest::IActions &a) { a.expect_eq(to_string(I32LessThanEqualUnsigned{}), "i32.le_u"); });

    s.add_test("i32_greater_than_equal_signed",
            [](etest::IActions &a) { a.expect_eq(to_string(I32GreaterThanEqualSigned{}), "i32.ge_s"); });

    s.add_test("i32_greater_than_equal_unsigned",
            [](etest::IActions &a) { a.expect_eq(to_string(I32GreaterThanEqualUnsigned{}), "i32.ge_u"); });

    s.add_test("i32_count_leading_zeros",
            [](etest::IActions &a) { a.expect_eq(to_string(I32CountLeadingZeros{}), "i32.clz"); });

    s.add_test("i32_count_trailing_zeros",
            [](etest::IActions &a) { a.expect_eq(to_string(I32CountTrailingZeros{}), "i32.ctz"); });

    s.add_test("i32_population_count",
            [](etest::IActions &a) { a.expect_eq(to_string(I32PopulationCount{}), "i32.popcnt"); });

    s.add_test("i32_add", [](etest::IActions &a) { a.expect_eq(to_string(I32Add{}), "i32.add"); });

    s.add_test("i32_subtract", [](etest::IActions &a) { a.expect_eq(to_string(I32Subtract{}), "i32.sub"); });

    s.add_test("i32_multiply", [](etest::IActions &a) { a.expect_eq(to_string(I32Multiply{}), "i32.mul"); });

    s.add_test("i32_divide_signed", [](etest::IActions &a) { a.expect_eq(to_string(I32DivideSigned{}), "i32.div_s"); });

    s.add_test("i32_divide_unsigned",
            [](etest::IActions &a) { a.expect_eq(to_string(I32DivideUnsigned{}), "i32.div_u"); });

    s.add_test("i32_remainder_signed",
            [](etest::IActions &a) { a.expect_eq(to_string(I32RemainderSigned{}), "i32.rem_s"); });

    s.add_test("i32_remainder_unsigned",
            [](etest::IActions &a) { a.expect_eq(to_string(I32RemainderUnsigned{}), "i32.rem_u"); });

    s.add_test("i32_and", [](etest::IActions &a) { a.expect_eq(to_string(I32And{}), "i32.and"); });

    s.add_test("i32_or", [](etest::IActions &a) { a.expect_eq(to_string(I32Or{}), "i32.or"); });

    s.add_test("i32_exlusive_or", [](etest::IActions &a) { a.expect_eq(to_string(I32ExclusiveOr{}), "i32.xor"); });

    s.add_test("i32_shift_left", [](etest::IActions &a) { a.expect_eq(to_string(I32ShiftLeft{}), "i32.shl"); });

    s.add_test("i32_shift_right_signed",
            [](etest::IActions &a) { a.expect_eq(to_string(I32ShiftRightSigned{}), "i32.shr_s"); });

    s.add_test("i32_shift_right_unsigned",
            [](etest::IActions &a) { a.expect_eq(to_string(I32ShiftRightUnsigned{}), "i32.shr_u"); });

    s.add_test("i32_rotate_left", [](etest::IActions &a) { a.expect_eq(to_string(I32RotateLeft{}), "i32.rotl"); });

    s.add_test("i32_rotate_right", [](etest::IActions &a) { a.expect_eq(to_string(I32RotateRight{}), "i32.rotr"); });

    s.add_test("i32_wrap_i64", [](etest::IActions &a) { a.expect_eq(to_string(I32WrapI64{}), "i32.wrap_i64"); });

    s.add_test("i32_truncate_f32_signed",
            [](etest::IActions &a) { a.expect_eq(to_string(I32TruncateF32Signed{}), "i32.trunc_f32_s"); });

    s.add_test("i32_truncate_f32_unsigned",
            [](etest::IActions &a) { a.expect_eq(to_string(I32TruncateF32Unsigned{}), "i32.trunc_f32_u"); });

    s.add_test("i32_truncate_f64_signed",
            [](etest::IActions &a) { a.expect_eq(to_string(I32TruncateF64Signed{}), "i32.trunc_f64_s"); });

    s.add_test("i32_truncate_f64_unsigned",
            [](etest::IActions &a) { a.expect_eq(to_string(I32TruncateF64Unsigned{}), "i32.trunc_f64_u"); });

    s.add_test("i32_reinterpret_f32",
            [](etest::IActions &a) { a.expect_eq(to_string(I32ReinterpretF32{}), "i32.reinterpret_f32"); });

    s.add_test("i32_extend8_signed",
            [](etest::IActions &a) { a.expect_eq(to_string(I32Extend8Signed{}), "i32.extend8_s"); });

    s.add_test("i32_extend16_signed",
            [](etest::IActions &a) { a.expect_eq(to_string(I32Extend16Signed{}), "i32.extend16_s"); });

    s.add_test("local_get", [](etest::IActions &a) { a.expect_eq(to_string(LocalGet{}), "local.get 0"); });

    s.add_test("local_set", [](etest::IActions &a) { a.expect_eq(to_string(LocalSet{}), "local.set 0"); });

    s.add_test("local_tee", [](etest::IActions &a) { a.expect_eq(to_string(LocalTee{}), "local.tee 0"); });

    s.add_test("i32_load", [](etest::IActions &a) {
        a.expect_eq(to_string(I32Load{32, 0}), "i32.load"); // natural alignment, offset 0
        a.expect_eq(to_string(I32Load{64, 0}), "i32.load align=64"); // 64-bit alignment for 32-bit load, offset 0
        a.expect_eq(to_string(I32Load{64, 3}), "i32.load offset=3 align=64"); // 64-bit alignment, offset 3
    });

    return s.run();
}
