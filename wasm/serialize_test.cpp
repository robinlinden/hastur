// SPDX-FileCopyrightText: 2024 David Zero <zero-one@zer0-one.net>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "wasm/instructions.h"
#include "wasm/serialize.h"
#include "wasm/wasm.h"

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

    s.add_test("i32_less_than_signed",
            [](etest::IActions &a) { a.expect_eq(to_string(I32LessThanSigned{}), "i32.lt_s"); });

    s.add_test("i32_add", [](etest::IActions &a) { a.expect_eq(to_string(I32Add{}), "i32.add"); });

    s.add_test("i32_sub", [](etest::IActions &a) { a.expect_eq(to_string(I32Sub{}), "i32.sub"); });

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
