// SPDX-FileCopyrightText: 2023-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "js/tokenizer.h"

#include "etest/etest2.h"

#include <optional>
#include <source_location>
#include <string_view>
#include <vector>

using namespace js::parse;

namespace {

void expect_tokens(etest::IActions &a,
        std::string_view input,
        std::vector<Token> tokens,
        std::source_location const &loc = std::source_location::current()) {
    tokens.emplace_back(Eof{});
    a.expect_eq(tokenize(input), tokens, std::nullopt, loc);
}

void expect_tokens(etest::IActions &a,
        std::string_view input,
        std::nullopt_t failure,
        std::source_location const &loc = std::source_location::current()) {
    a.expect_eq(tokenize(input), failure, std::nullopt, loc);
}

} // namespace

int main() {
    etest::Suite s{};

    s.add_test("int literal", [](etest::IActions &a) {
        expect_tokens(a, "13", {IntLiteral{13}});
        expect_tokens(a, "0", {IntLiteral{0}});

        a.expect_eq(tokenize("2147483647"), std::vector<Token>{IntLiteral{2147483647}, Eof{}});
        a.expect_eq(tokenize("2147483648"), std::nullopt);
    });

    s.add_test("identifier", [](etest::IActions &a) {
        expect_tokens(a, "hello", {Identifier{"hello"}}); //
    });

    s.add_test("identifiers w/ whitespace", [](etest::IActions &a) {
        expect_tokens(a, " lol no ", {Identifier{"lol"}, Identifier{"no"}}); //
    });

    s.add_test("function call", [](etest::IActions &a) {
        expect_tokens(a, "func();", {Identifier{"func"}, LParen{}, RParen{}, Semicolon{}}); //
    });

    s.add_test("function call w/ whitespace", [](etest::IActions &a) {
        expect_tokens(a, "func  (   )    ;", {Identifier{"func"}, LParen{}, RParen{}, Semicolon{}}); //
    });

    s.add_test("function call w/ numeric argument", [](etest::IActions &a) {
        expect_tokens(a, "func(9)", {Identifier{"func"}, LParen{}, IntLiteral{9}, RParen{}});
        expect_tokens(a, "func( 9 )", {Identifier{"func"}, LParen{}, IntLiteral{9}, RParen{}});
    });

    s.add_test("function call w/ multiple arguments", [](etest::IActions &a) {
        expect_tokens(a, "nh(5, 20)", {Identifier{"nh"}, LParen{}, IntLiteral{5}, Comma{}, IntLiteral{20}, RParen{}});
    });

    s.add_test("unsupported input", [](etest::IActions &a) {
        a.expect_eq(tokenize("~"), std::nullopt); //
    });

    s.add_test("period", [](etest::IActions &a) {
        expect_tokens(a, ".", {Period{}});
        expect_tokens(a, "hello.world", {Identifier{"hello"}, Period{}, Identifier{"world"}});
    });

    s.add_test("equals", [](etest::IActions &a) {
        expect_tokens(a, "=", {Equals{}});
        expect_tokens(a, "hello = 5", {Identifier{"hello"}, Equals{}, IntLiteral{5}});
        expect_tokens(a, "hello=5", {Identifier{"hello"}, Equals{}, IntLiteral{5}});
    });

    s.add_test("braces", [](etest::IActions &a) {
        expect_tokens(a, "{}", {LBrace{}, RBrace{}});
        expect_tokens(a, "{ }", {LBrace{}, RBrace{}});
        expect_tokens(a, "{ hello }", {LBrace{}, Identifier{"hello"}, RBrace{}});
        expect_tokens(a, "{ hello; }", {LBrace{}, Identifier{"hello"}, Semicolon{}, RBrace{}});
    });

    s.add_test("brackets", [](etest::IActions &a) {
        expect_tokens(a, "[]", {LBracket{}, RBracket{}});
        expect_tokens(a, "[ ]", {LBracket{}, RBracket{}});
        expect_tokens(a, "[ hello ]", {LBracket{}, Identifier{"hello"}, RBracket{}});
        expect_tokens(a, "[ hello; ]", {LBracket{}, Identifier{"hello"}, Semicolon{}, RBracket{}});
    });

    s.add_test("maths", [](etest::IActions &a) {
        expect_tokens(a, "5 + 6", {IntLiteral{5}, Plus{}, IntLiteral{6}});
        expect_tokens(a, "5*6", {IntLiteral{5}, Asterisk{}, IntLiteral{6}});
        expect_tokens(a, "5 * 6", {IntLiteral{5}, Asterisk{}, IntLiteral{6}});
        expect_tokens(a, "5+6*7", {IntLiteral{5}, Plus{}, IntLiteral{6}, Asterisk{}, IntLiteral{7}});
    });

    s.add_test("comments", [](etest::IActions &a) {
        expect_tokens(a, "/* comment */", {Comment{" comment "}});
        expect_tokens(a, "/*comment*//* comment */", {Comment{"comment"}, Comment{" comment "}});
        expect_tokens(a, "/* comment */5", {Comment{" comment "}, IntLiteral{5}});
        expect_tokens(a, "/* comment */ 5 /* comment */", {Comment{" comment "}, IntLiteral{5}, Comment{" comment "}});
        expect_tokens(a,
                "/* comment */5/* comment */ 6",
                {Comment{" comment "}, IntLiteral{5}, Comment{" comment "}, IntLiteral{6}});

        a.expect_eq(tokenize("/"), std::nullopt);
        expect_tokens(a, "/*asdf* ** ** ****", {Comment{"asdf* ** ** ****"}});
    });

    s.add_test("string literal", [](etest::IActions &a) {
        expect_tokens(a, "'hello'", {StringLiteral{"hello"}});
        expect_tokens(a, "\"hello\"", {StringLiteral{"hello"}});
        expect_tokens(a, "''", {StringLiteral{}});
        expect_tokens(a, "\"\"", {StringLiteral{}});
        expect_tokens(a, "'asdf", std::nullopt);
        expect_tokens(a, "\"asdf", std::nullopt);
    });

    return s.run();
}
