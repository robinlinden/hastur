// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "js/ast2.h"

#include "etest/etest.h"

using namespace js::ast2;
using etest::expect_eq;

int main() {
    etest::test("literals", [] {
        AstExecutor e;
        expect_eq(e.execute(NumericLiteral{5.}), Value{5.});
        expect_eq(e.execute(StringLiteral{"hello"}), Value{"hello"});
    });

    return etest::run_all_tests();
}
