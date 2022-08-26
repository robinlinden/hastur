// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "js/ast.h"

#include "etest/etest.h"

using namespace js::ast;
using etest::expect_eq;

int main() {
    etest::test("Value: as_bool", [] {
        expect_eq(Value{""}.as_bool(), false);
        expect_eq(Value{0}.as_bool(), false);
        expect_eq(Value{-0}.as_bool(), false);
        expect_eq(Value{}.as_bool(), false);

        expect_eq(Value{" "}.as_bool(), true);
        expect_eq(Value{1}.as_bool(), true);
        expect_eq(Value{-0.001}.as_bool(), true);
        expect_eq(Value{std::vector<Value>{}}.as_bool(), true);
    });

    return etest::run_all_tests();
}
