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
    etest::test("int literal", [] {
        expect_eq(tokenize("13"), Tokens{IntLiteral{13}, Eof{}});
        expect_eq(tokenize("0"), Tokens{IntLiteral{0}, Eof{}});
    });

    etest::test("identifier", [] {
        expect_eq(tokenize("hello"), Tokens{Identifier{"hello"}, Eof{}}); //
    });

    etest::test("identifiers w/ whitespace", [] {
        expect_eq(tokenize(" lol no "), Tokens{Identifier{"lol"}, Identifier{"no"}, Eof{}}); //
    });

    etest::test("function call", [] {
        expect_eq(tokenize("func();"), Tokens{Identifier{"func"}, LParen{}, RParen{}, Semicolon{}, Eof{}}); //
    });

    etest::test("function call w/ whitespace", [] {
        expect_eq(tokenize("func  (   )    ;"), Tokens{Identifier{"func"}, LParen{}, RParen{}, Semicolon{}, Eof{}}); //
    });

    etest::test("function call w/ numeric argument", [] {
        expect_eq(tokenize("func(9)"), Tokens{Identifier{"func"}, LParen{}, IntLiteral{9}, RParen{}, Eof{}});
        expect_eq(tokenize("func( 9 )"), Tokens{Identifier{"func"}, LParen{}, IntLiteral{9}, RParen{}, Eof{}});
    });

    etest::test("function call w/ multiple arguments", [] {
        expect_eq(tokenize("nh(5, 20)"),
                Tokens{Identifier{"nh"}, LParen{}, IntLiteral{5}, Comma{}, IntLiteral{20}, RParen{}, Eof{}});
    });

    return etest::run_all_tests();
}
