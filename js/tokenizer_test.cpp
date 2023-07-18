// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "js/tokenizer.h"

#include "etest/etest.h"

#include <vector>

using namespace js::parse;

using etest::expect_eq;

using Tokens = std::vector<Token>;

int main() {
    etest::test("identifier", [] {
        expect_eq(tokenize("hello"), Tokens{Identifier{"hello"}, Eof{}}); //
    });

    etest::test("function call", [] {
        expect_eq(tokenize("func();"), Tokens{Identifier{"func"}, LParen{}, RParen{}, Semicolon{}, Eof{}}); //
    });

    return etest::run_all_tests();
}
