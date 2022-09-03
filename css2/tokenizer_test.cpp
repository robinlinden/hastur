// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "css2/tokenizer.h"

#include "etest/etest.h"

#include <vector>

using etest::expect;
using etest::expect_eq;
using etest::require;

using namespace css2;

namespace {

class TokenizerOutput {
public:
    ~TokenizerOutput() {
        expect(tokens.empty(), loc);
        expect(errors.empty(), loc);
    }

    std::vector<Token> tokens;
    std::vector<ParseError> errors;
    etest::source_location loc;
};

TokenizerOutput run_tokenizer(std::string_view input, etest::source_location loc = etest::source_location::current()) {
    std::vector<Token> tokens;
    std::vector<ParseError> errors;
    Tokenizer{input,
            [&](Token &&t) { tokens.push_back(std::move(t)); },
            [&](ParseError e) {
                errors.push_back(e);
            }}
            .run();
    return {std::move(tokens), std::move(errors), std::move(loc)};
}

void expect_token(
        TokenizerOutput &output, Token t, etest::source_location const &loc = etest::source_location::current()) {
    require(!output.tokens.empty(), loc);
    expect_eq(output.tokens.front(), t, loc);
    output.tokens.erase(begin(output.tokens));
}

void expect_error(
        TokenizerOutput &output, ParseError e, etest::source_location const &loc = etest::source_location::current()) {
    require(!output.errors.empty(), loc);
    expect_eq(output.errors.front(), e, loc);
    output.errors.erase(begin(output.errors));
}

} // namespace

int main() {
    etest::test("delimiter", [] {
        auto output = run_tokenizer("a");

        expect_token(output, DelimToken{'a'});
    });

    etest::test("comment", [] {
        auto output = run_tokenizer("/* foo */");

        expect(output.tokens.empty());
    });

    etest::test("comment with asterisks", [] {
        auto output = run_tokenizer("/*****/");

        expect(output.tokens.empty());
    });

    etest::test("comment almost started", [] {
        auto output = run_tokenizer("/a");

        expect_token(output, DelimToken{'/'});
        expect_token(output, DelimToken{'a'});
    });

    etest::test("delimiter after comment", [] {
        auto output = run_tokenizer("/*/*/,");

        expect_token(output, DelimToken{','});
    });

    etest::test("eof in comment", [] {
        auto output = run_tokenizer("/* foo");

        expect_error(output, ParseError::EofInComment);
    });

    etest::test("eof at comment ending", [] {
        auto output = run_tokenizer("/* foo *");

        expect_error(output, ParseError::EofInComment);
    });

    etest::test("space and comments", [] {
        auto output = run_tokenizer(" /* */   /**/");

        expect_token(output, WhitespaceToken{});
        expect_token(output, WhitespaceToken{});
    });

    etest::test("end with one tab", [] {
        auto output = run_tokenizer("a\t");

        expect_token(output, DelimToken{'a'});
        expect_token(output, WhitespaceToken{});
    });

    etest::test("end with two tabs", [] {
        auto output = run_tokenizer("a\t\t");

        expect_token(output, DelimToken{'a'});
        expect_token(output, WhitespaceToken{});
    });

    etest::test("end with one line feed", [] {
        auto output = run_tokenizer("a\n");

        expect_token(output, DelimToken{'a'});
        expect_token(output, WhitespaceToken{});
    });

    etest::test("end with two line feeds", [] {
        auto output = run_tokenizer("a\n\n");

        expect_token(output, DelimToken{'a'});
        expect_token(output, WhitespaceToken{});
    });
    return etest::run_all_tests();
}
