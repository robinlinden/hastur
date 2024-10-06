// SPDX-FileCopyrightText: 2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "wasm/interpreter.h"

#include "wasm/instructions.h"

#include "etest/etest2.h"

#include <cstddef>
#include <cstdint>
#include <variant>

int main() {
    using namespace wasm::instructions;
    using wasm::Interpreter;
    etest::Suite s{};

    s.add_test("i32.const", [](etest::IActions &a) {
        Interpreter i;
        i.interpret(I32Const{42});

        a.require_eq(i.stack.size(), std::size_t{1});
        a.expect_eq(std::get<std::int32_t>(i.stack.back()), 42);
    });

    s.add_test("i32.add", [](etest::IActions &a) {
        Interpreter i;
        i.interpret(I32Const{20});
        i.interpret(I32Const{22});
        i.interpret(I32Add{});

        a.require_eq(i.stack.size(), std::size_t{1});
        a.expect_eq(std::get<std::int32_t>(i.stack.back()), 42);
    });

    s.add_test("local.get", [](etest::IActions &a) {
        Interpreter i;
        i.locals.push_back(42);
        i.interpret(LocalGet{0});

        a.require_eq(i.stack.size(), std::size_t{1});
        a.expect_eq(std::get<std::int32_t>(i.stack.back()), 42);
    });

    s.add_test("local.set", [](etest::IActions &a) {
        Interpreter i;
        i.locals.push_back(42);
        i.interpret(I32Const{24});
        i.interpret(LocalSet{0});

        a.expect_eq(i.stack.size(), std::size_t{0});
        a.require_eq(i.locals.size(), std::size_t{1});
        a.expect_eq(std::get<std::int32_t>(i.locals.back()), 24);
    });

    s.add_test("local.tee", [](etest::IActions &a) {
        Interpreter i;
        i.locals.push_back(42);
        i.interpret(I32Const{24});
        i.interpret(LocalTee{0});

        a.require_eq(i.stack.size(), std::size_t{1});
        a.expect_eq(std::get<std::int32_t>(i.stack.back()), 24);
        a.require_eq(i.locals.size(), std::size_t{1});
        a.expect_eq(std::get<std::int32_t>(i.locals.back()), 24);
    });

    return s.run();
}
