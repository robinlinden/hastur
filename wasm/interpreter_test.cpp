// SPDX-FileCopyrightText: 2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "wasm/interpreter.h"

#include "etest/etest2.h"

int main() {
    using namespace wasm::instructions;
    etest::Suite s{"wasm::instructions"};

    s.add_test("i32.const", [](etest::IActions &a) {
        wasm::Interpreter i;
        i.interpret(I32Const{42});
        a.require_eq(i.stack.size(), std::size_t{1});
        a.expect_eq(std::get<std::int32_t>(i.stack.back()), 42);
    });

    return s.run();
}
