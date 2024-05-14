// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "js/tokenizer.h"

#include "etest/etest.h"

#include <optional>
#include <source_location>
#include <string_view>
#include <vector>

using namespace js::parse;

namespace {

void expect_tokens(std::string_view input,
        std::vector<Token> tokens,
        std::source_location const &loc = std::source_location::current()) {
    tokens.push_back(Eof{});
    etest::expect_eq(tokenize(input), tokens, std::nullopt, loc);
}

} // namespace

int main() {
    etest::test("int literal", [] {
        expect_tokens("13", {IntLiteral{13}});
        expect_tokens("0", {IntLiteral{0}});
    });

    etest::test("identifier", [] {
        expect_tokens("hello", {Identifier{"hello"}}); //
    });

    etest::test("identifiers w/ whitespace", [] {
        expect_tokens(" lol no ", {Identifier{"lol"}, Identifier{"no"}}); //
    });

    etest::test("function call", [] {
        expect_tokens("func();", {Identifier{"func"}, LParen{}, RParen{}, Semicolon{}}); //
    });

    etest::test("function call w/ whitespace", [] {
        expect_tokens("func  (   )    ;", {Identifier{"func"}, LParen{}, RParen{}, Semicolon{}}); //
    });

    etest::test("function call w/ numeric argument", [] {
        expect_tokens("func(9)", {Identifier{"func"}, LParen{}, IntLiteral{9}, RParen{}});
        expect_tokens("func( 9 )", {Identifier{"func"}, LParen{}, IntLiteral{9}, RParen{}});
    });

    etest::test("function call w/ multiple arguments", [] {
        expect_tokens("nh(5, 20)", {Identifier{"nh"}, LParen{}, IntLiteral{5}, Comma{}, IntLiteral{20}, RParen{}});
    });

    return etest::run_all_tests();
}
