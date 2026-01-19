// SPDX-FileCopyrightText: 2024-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "wasm/interpreter.h"

#include "wasm/instructions.h"

#include "etest/etest2.h"

#include <tl/expected.hpp>

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
        a.expect_eq(i.interpret(wasm::instructions::End{}), tl::unexpected{wasm::Trap::UnhandledInstruction});
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
        auto res = i.run({{I32Const{42}}});
        a.expect_eq(res, wasm::Interpreter::Value{42});
    });

    s.add_test("i32.lt_s", [](etest::IActions &a) {
        Interpreter i;
        // Less.
        auto res = i.run({{I32Const{10}, I32Const{20}, I32LessThanSigned{}}});
        a.expect_eq(res, wasm::Interpreter::Value{1});
        i.stack.clear();

        // Greater.
        res = i.run({{I32Const{20}, I32Const{10}, I32LessThanSigned{}}});
        a.expect_eq(res, wasm::Interpreter::Value{0});
        i.stack.clear();

        // Same.
        res = i.run({{I32Const{10}, I32Const{10}, I32LessThanSigned{}}});
        a.expect_eq(res, wasm::Interpreter::Value{0});
    });

    s.add_test("i32.gt_s", [](etest::IActions &a) {
        Interpreter i;
        // Less.
        auto res = i.run({{I32Const{10}, I32Const{20}, I32GreaterThanSigned{}}});
        a.expect_eq(res, wasm::Interpreter::Value{0});
        i.stack.clear();

        // Greater.
        res = i.run({{I32Const{20}, I32Const{10}, I32GreaterThanSigned{}}});
        a.expect_eq(res, wasm::Interpreter::Value{1});
        i.stack.clear();

        // Same.
        res = i.run({{I32Const{10}, I32Const{10}, I32GreaterThanSigned{}}});
        a.expect_eq(res, wasm::Interpreter::Value{0});
    });

    s.add_test("i32.le_s", [](etest::IActions &a) {
        Interpreter i;
        // Less.
        auto res = i.run({{I32Const{10}, I32Const{20}, I32LessThanEqualSigned{}}});
        a.expect_eq(res, wasm::Interpreter::Value{1});
        i.stack.clear();

        // Greater.
        res = i.run({{I32Const{20}, I32Const{10}, I32LessThanEqualSigned{}}});
        a.expect_eq(res, wasm::Interpreter::Value{0});
        i.stack.clear();

        // Same.
        res = i.run({{I32Const{10}, I32Const{10}, I32LessThanEqualSigned{}}});
        a.expect_eq(res, wasm::Interpreter::Value{1});
    });

    s.add_test("i32.ge_s", [](etest::IActions &a) {
        Interpreter i;
        // Less.
        auto res = i.run({{I32Const{10}, I32Const{20}, I32GreaterThanEqualSigned{}}});
        a.expect_eq(res, wasm::Interpreter::Value{0});
        i.stack.clear();

        // Greater.
        res = i.run({{I32Const{20}, I32Const{10}, I32GreaterThanEqualSigned{}}});
        a.expect_eq(res, wasm::Interpreter::Value{1});
        i.stack.clear();

        // Same.
        res = i.run({{I32Const{10}, I32Const{10}, I32GreaterThanEqualSigned{}}});
        a.expect_eq(res, wasm::Interpreter::Value{1});
    });

    s.add_test("i32.add", [](etest::IActions &a) {
        Interpreter i;
        auto res = i.run({{I32Const{20}, I32Const{22}, I32Add{}}});
        a.expect_eq(res, wasm::Interpreter::Value{42});
    });

    s.add_test("i32.sub", [](etest::IActions &a) {
        Interpreter i;
        auto res = i.run({{I32Const{100}, I32Const{58}, I32Subtract{}}});
        a.expect_eq(res, wasm::Interpreter::Value{42});
    });

    s.add_test("i32.and", [](etest::IActions &a) {
        Interpreter i;
        auto res = i.run({{I32Const{0b1100}, I32Const{0b1010}, I32And{}}});
        a.expect_eq(res, wasm::Interpreter::Value{0b1000});
    });

    s.add_test("i32.or", [](etest::IActions &a) {
        Interpreter i;
        auto res = i.run({{I32Const{0b1100}, I32Const{0b1010}, I32Or{}}});
        a.expect_eq(res, wasm::Interpreter::Value{0b1110});
    });

    s.add_test("i32.xor", [](etest::IActions &a) {
        Interpreter i;
        auto res = i.run({{I32Const{0b1100}, I32Const{0b1010}, I32ExclusiveOr{}}});
        a.expect_eq(res, wasm::Interpreter::Value{0b0110});
    });

    s.add_test("local.get", [](etest::IActions &a) {
        Interpreter i;
        i.locals.emplace_back(42);
        auto res = i.run({{LocalGet{0}}});
        a.expect_eq(res, wasm::Interpreter::Value{42});
    });

    s.add_test("local.set", [](etest::IActions &a) {
        Interpreter i;
        i.locals.emplace_back(42);
        auto res = i.run({{I32Const{24}, LocalSet{0}}});

        a.expect_eq(i.stack.size(), std::size_t{0});
        a.require_eq(i.locals.size(), std::size_t{1});
        a.expect_eq(std::get<std::int32_t>(i.locals.back()), 24);
        a.expect_eq(res, std::nullopt);
    });

    s.add_test("local.tee", [](etest::IActions &a) {
        Interpreter i;
        i.locals.emplace_back(42);
        auto res = i.run({{I32Const{24}, LocalTee{0}}});

        a.require_eq(i.stack.size(), std::size_t{1});
        a.expect_eq(std::get<std::int32_t>(i.stack.back()), 24);
        a.require_eq(i.locals.size(), std::size_t{1});
        a.expect_eq(std::get<std::int32_t>(i.locals.back()), 24);
        a.expect_eq(res, wasm::Interpreter::Value{24});
    });

    s.add_test("global.get", [](etest::IActions &a) {
        Interpreter i;
        i.globals.emplace_back(84);
        auto res = i.run({{GlobalGet{0}}});

        a.require_eq(i.stack.size(), std::size_t{1});
        a.expect_eq(std::get<std::int32_t>(i.stack.back()), 84);
        a.expect_eq(res, wasm::Interpreter::Value{84});
    });

    s.add_test("global.set", [](etest::IActions &a) {
        Interpreter i;
        i.globals.emplace_back(84);
        auto res = i.run({{I32Const{21}, GlobalSet{0}}});

        a.expect_eq(i.stack.size(), std::size_t{0});
        a.require_eq(i.globals.size(), std::size_t{1});
        a.expect_eq(std::get<std::int32_t>(i.globals.back()), 21);
        a.expect_eq(res, std::nullopt);
    });

    s.add_test("i32.load", [](etest::IActions &a) {
        Interpreter i;
        i.memory.resize(8);
        // Little-endian 42.
        i.memory[4] = 42;
        i.memory[5] = 0;
        i.memory[6] = 0;
        i.memory[7] = 0;

        auto res = i.run({{I32Const{4}, I32Load{{0, 0}}}});
        a.expect_eq(res, wasm::Interpreter::Value{42});

        // Out-of-bounds read.
        res = i.run({{I32Const{4}, I32Load{{0, 100}}}});
        a.expect_eq(res, tl::unexpected{wasm::Trap::MemoryAccessOutOfBounds});
    });

    s.add_test("i32.store", [](etest::IActions &a) {
        Interpreter i;
        i.memory.resize(8);

        // Store 42 at address 4.
        auto res = i.run({{I32Const{4}, I32Const{42}, I32Store{{0, 0}}}});
        a.expect_eq(res, std::nullopt);

        a.expect_eq(i.memory[4], 42);
        a.expect_eq(i.memory[5], 0);
        a.expect_eq(i.memory[6], 0);
        a.expect_eq(i.memory[7], 0);

        a.expect_eq(i.stack.size(), std::size_t{0});

        // and load the value again.
        res = i.run({{I32Const{4}, I32Load{{0, 0}}}});
        a.expect_eq(res, wasm::Interpreter::Value{42});

        // Out-of-bounds write.
        res = i.run({{I32Const{5}, I32Const{42}, I32Store{{0, 0}}}});
        a.expect_eq(res, tl::unexpected{wasm::Trap::MemoryAccessOutOfBounds});
    });

    return s.run();
}
