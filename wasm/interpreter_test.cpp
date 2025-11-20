// SPDX-FileCopyrightText: 2024-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "wasm/interpreter.h"

#include "wasm/instructions.h"

#include "etest/etest2.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

int main() {
    using namespace wasm::instructions;
    using wasm::Interpreter;
    etest::Suite s{};

    s.add_test("unhandled instruction", [](etest::IActions &a) {
        Interpreter i;
        i.interpret(wasm::instructions::End{});
        a.expect_eq(i, Interpreter{});
    });

    s.add_test("Interpreter::run", [](etest::IActions &a) {
        Interpreter i;
        auto result = i.run(std::span<Instruction const>{{
                I32Const{42},
                I32Const{0},
                I32Add{},
        }});
        a.expect_eq(result, wasm::Interpreter::Value{42});

        i = Interpreter{};
        i.locals.resize(1);
        result = i.run(std::span<Instruction const>{{
                I32Const{10},
                LocalSet{0},
        }});
        a.expect_eq(result, std::nullopt);
    });

    s.add_test("i32.const", [](etest::IActions &a) {
        Interpreter i;
        i.interpret(I32Const{42});

        a.require_eq(i.stack.size(), std::size_t{1});
        a.expect_eq(std::get<std::int32_t>(i.stack.back()), 42);
    });

    s.add_test("i32.lt_s", [](etest::IActions &a) {
        Interpreter i;
        // Less.
        i.interpret(I32Const{10});
        i.interpret(I32Const{20});
        i.interpret(I32LessThanSigned{});
        a.require_eq(i.stack.size(), std::size_t{1});
        a.expect_eq(std::get<std::int32_t>(i.stack.back()), 1);
        i.stack.clear();

        // Greater.
        i.interpret(I32Const{20});
        i.interpret(I32Const{10});
        i.interpret(I32LessThanSigned{});
        a.require_eq(i.stack.size(), std::size_t{1});
        a.expect_eq(std::get<std::int32_t>(i.stack.back()), 0);
        i.stack.clear();

        // Same.
        i.interpret(I32Const{10});
        i.interpret(I32Const{10});
        i.interpret(I32LessThanSigned{});
        a.require_eq(i.stack.size(), std::size_t{1});
        a.expect_eq(std::get<std::int32_t>(i.stack.back()), 0);
    });

    s.add_test("i32.gt_s", [](etest::IActions &a) {
        Interpreter i;
        // Less.
        i.interpret(I32Const{10});
        i.interpret(I32Const{20});
        i.interpret(I32GreaterThanSigned{});
        a.require_eq(i.stack.size(), std::size_t{1});
        a.expect_eq(std::get<std::int32_t>(i.stack.back()), 0);
        i.stack.clear();

        // Greater.
        i.interpret(I32Const{20});
        i.interpret(I32Const{10});
        i.interpret(I32GreaterThanSigned{});
        a.require_eq(i.stack.size(), std::size_t{1});
        a.expect_eq(std::get<std::int32_t>(i.stack.back()), 1);
        i.stack.clear();

        // Same.
        i.interpret(I32Const{10});
        i.interpret(I32Const{10});
        i.interpret(I32GreaterThanSigned{});
        a.require_eq(i.stack.size(), std::size_t{1});
        a.expect_eq(std::get<std::int32_t>(i.stack.back()), 0);
    });

    s.add_test("i32.le_s", [](etest::IActions &a) {
        Interpreter i;
        // Less.
        i.interpret(I32Const{10});
        i.interpret(I32Const{20});
        i.interpret(I32LessThanEqualSigned{});
        a.require_eq(i.stack.size(), std::size_t{1});
        a.expect_eq(std::get<std::int32_t>(i.stack.back()), 1);
        i.stack.clear();

        // Greater.
        i.interpret(I32Const{20});
        i.interpret(I32Const{10});
        i.interpret(I32LessThanEqualSigned{});
        a.require_eq(i.stack.size(), std::size_t{1});
        a.expect_eq(std::get<std::int32_t>(i.stack.back()), 0);
        i.stack.clear();

        // Same.
        i.interpret(I32Const{10});
        i.interpret(I32Const{10});
        i.interpret(I32LessThanEqualSigned{});
        a.require_eq(i.stack.size(), std::size_t{1});
        a.expect_eq(std::get<std::int32_t>(i.stack.back()), 1);
    });

    s.add_test("i32.ge_s", [](etest::IActions &a) {
        Interpreter i;
        // Less.
        i.interpret(I32Const{10});
        i.interpret(I32Const{20});
        i.interpret(I32GreaterThanEqualSigned{});
        a.require_eq(i.stack.size(), std::size_t{1});
        a.expect_eq(std::get<std::int32_t>(i.stack.back()), 0);
        i.stack.clear();

        // Greater.
        i.interpret(I32Const{20});
        i.interpret(I32Const{10});
        i.interpret(I32GreaterThanEqualSigned{});
        a.require_eq(i.stack.size(), std::size_t{1});
        a.expect_eq(std::get<std::int32_t>(i.stack.back()), 1);
        i.stack.clear();

        // Same.
        i.interpret(I32Const{10});
        i.interpret(I32Const{10});
        i.interpret(I32GreaterThanEqualSigned{});
        a.require_eq(i.stack.size(), std::size_t{1});
        a.expect_eq(std::get<std::int32_t>(i.stack.back()), 1);
    });

    s.add_test("i32.add", [](etest::IActions &a) {
        Interpreter i;
        i.interpret(I32Const{20});
        i.interpret(I32Const{22});
        i.interpret(I32Add{});

        a.require_eq(i.stack.size(), std::size_t{1});
        a.expect_eq(std::get<std::int32_t>(i.stack.back()), 42);
    });

    s.add_test("i32.sub", [](etest::IActions &a) {
        Interpreter i;
        i.interpret(I32Const{100});
        i.interpret(I32Const{58});
        i.interpret(I32Subtract{});

        a.require_eq(i.stack.size(), std::size_t{1});
        a.expect_eq(std::get<std::int32_t>(i.stack.back()), 42);
    });

    s.add_test("i32.and", [](etest::IActions &a) {
        Interpreter i;
        i.interpret(I32Const{0b1100});
        i.interpret(I32Const{0b1010});
        i.interpret(I32And{});

        a.require_eq(i.stack.size(), std::size_t{1});
        a.expect_eq(std::get<std::int32_t>(i.stack.back()), 0b1000);
    });

    s.add_test("i32.or", [](etest::IActions &a) {
        Interpreter i;
        i.interpret(I32Const{0b1100});
        i.interpret(I32Const{0b1010});
        i.interpret(I32Or{});

        a.require_eq(i.stack.size(), std::size_t{1});
        a.expect_eq(std::get<std::int32_t>(i.stack.back()), 0b1110);
    });

    s.add_test("i32.xor", [](etest::IActions &a) {
        Interpreter i;
        i.interpret(I32Const{0b1100});
        i.interpret(I32Const{0b1010});
        i.interpret(I32ExclusiveOr{});

        a.require_eq(i.stack.size(), std::size_t{1});
        a.expect_eq(std::get<std::int32_t>(i.stack.back()), 0b0110);
    });

    s.add_test("local.get", [](etest::IActions &a) {
        Interpreter i;
        i.locals.emplace_back(42);
        i.interpret(LocalGet{0});

        a.require_eq(i.stack.size(), std::size_t{1});
        a.expect_eq(std::get<std::int32_t>(i.stack.back()), 42);
    });

    s.add_test("local.set", [](etest::IActions &a) {
        Interpreter i;
        i.locals.emplace_back(42);
        i.interpret(I32Const{24});
        i.interpret(LocalSet{0});

        a.expect_eq(i.stack.size(), std::size_t{0});
        a.require_eq(i.locals.size(), std::size_t{1});
        a.expect_eq(std::get<std::int32_t>(i.locals.back()), 24);
    });

    s.add_test("local.tee", [](etest::IActions &a) {
        Interpreter i;
        i.locals.emplace_back(42);
        i.interpret(I32Const{24});
        i.interpret(LocalTee{0});

        a.require_eq(i.stack.size(), std::size_t{1});
        a.expect_eq(std::get<std::int32_t>(i.stack.back()), 24);
        a.require_eq(i.locals.size(), std::size_t{1});
        a.expect_eq(std::get<std::int32_t>(i.locals.back()), 24);
    });

    s.add_test("global.get", [](etest::IActions &a) {
        Interpreter i;
        i.globals.emplace_back(84);
        i.interpret(GlobalGet{0});

        a.require_eq(i.stack.size(), std::size_t{1});
        a.expect_eq(std::get<std::int32_t>(i.stack.back()), 84);
    });

    s.add_test("global.set", [](etest::IActions &a) {
        Interpreter i;
        i.globals.emplace_back(84);
        i.interpret(I32Const{21});
        i.interpret(GlobalSet{0});

        a.expect_eq(i.stack.size(), std::size_t{0});
        a.require_eq(i.globals.size(), std::size_t{1});
        a.expect_eq(std::get<std::int32_t>(i.globals.back()), 21);
    });

    s.add_test("i32.load", [](etest::IActions &a) {
        Interpreter i;
        i.memory.resize(8);
        // Little-endian 42.
        i.memory[4] = 42;
        i.memory[5] = 0;
        i.memory[6] = 0;
        i.memory[7] = 0;

        i.interpret(I32Const{4});
        i.interpret(I32Load{{0, 0}});

        a.require_eq(i.stack.size(), std::size_t{1});
        a.expect_eq(std::get<std::int32_t>(i.stack.back()), 42);
    });

    s.add_test("i32.store", [](etest::IActions &a) {
        Interpreter i;
        i.memory.resize(8);

        // Store 42 at address 4.
        i.interpret(I32Const{4});
        i.interpret(I32Const{42});
        i.interpret(I32Store{{0, 0}});

        a.expect_eq(i.memory[4], 42);
        a.expect_eq(i.memory[5], 0);
        a.expect_eq(i.memory[6], 0);
        a.expect_eq(i.memory[7], 0);

        a.expect_eq(i.stack.size(), std::size_t{0});

        // and load the value again.
        i.interpret(I32Const{4});
        i.interpret(I32Load{{0, 0}});
        a.require_eq(i.stack.size(), std::size_t{1});
        a.expect_eq(std::get<std::int32_t>(i.stack.back()), 42);
    });

    return s.run();
}
