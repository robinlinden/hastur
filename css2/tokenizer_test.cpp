// SPDX-FileCopyrightText: 2021-2024 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "css2/tokenizer.h"

#include "css2/token.h"

#include "etest/etest.h"

#include <source_location>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using etest::expect;
using etest::expect_eq;
using etest::require;

using namespace css2;
using namespace std::literals;

namespace {

constexpr char const *kReplacementCharacter = "\xef\xbf\xbd";

class TokenizerOutput {
public:
    ~TokenizerOutput() {
        expect(tokens.empty(), "Not all tokens were handled", loc);
        expect(errors.empty(), "Not all errors were handled", loc);
    }

    std::vector<Token> tokens;
    std::vector<ParseError> errors;
    std::source_location loc;
};

TokenizerOutput run_tokenizer(std::string_view input, std::source_location loc = std::source_location::current()) {
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
        TokenizerOutput &output, Token const &t, std::source_location const &loc = std::source_location::current()) {
    require(!output.tokens.empty(), "Unexpected end of token list", loc);
    expect_eq(output.tokens.front(), t, {}, loc);
    output.tokens.erase(begin(output.tokens));
}

void expect_error(
        TokenizerOutput &output, ParseError e, std::source_location const &loc = std::source_location::current()) {
    require(!output.errors.empty(), "Unexpected end of error list", loc);
    expect_eq(output.errors.front(), e, {}, loc);
    output.errors.erase(begin(output.errors));
}

} // namespace

int main() {
    etest::test("delimiter", [] {
        auto output = run_tokenizer("?");

        expect_token(output, DelimToken{'?'});
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
        expect_token(output, IdentToken{"a"});
    });

    etest::test("delimiter after comment", [] {
        auto output = run_tokenizer("/*/*/?");

        expect_token(output, DelimToken{'?'});
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

        expect_token(output, IdentToken{"a"});
        expect_token(output, WhitespaceToken{});
    });

    etest::test("end with two tabs", [] {
        auto output = run_tokenizer("a\t\t");

        expect_token(output, IdentToken{"a"});
        expect_token(output, WhitespaceToken{});
    });

    etest::test("end with one line feed", [] {
        auto output = run_tokenizer("a\n");

        expect_token(output, IdentToken{"a"});
        expect_token(output, WhitespaceToken{});
    });

    etest::test("end with two line feeds", [] {
        auto output = run_tokenizer("a\n\n");

        expect_token(output, IdentToken{"a"});
        expect_token(output, WhitespaceToken{});
    });

    etest::test("single quoted string", [] {
        auto output = run_tokenizer("'this is a string'");

        expect_token(output, StringToken{"this is a string"});
    });

    etest::test("double quoted string", [] {
        auto output = run_tokenizer(R"("this is a string")");

        expect_token(output, StringToken{"this is a string"});
    });

    etest::test("eof in string", [] {
        auto output = run_tokenizer(R"("this is a)");

        expect_error(output, ParseError::EofInString);
        expect_token(output, StringToken{"this is a"});
    });

    etest::test("newline in string", [] {
        auto output = run_tokenizer("\"this is a\n");

        expect_error(output, ParseError::NewlineInString);
        expect_token(output, BadStringToken{});
        expect_token(output, WhitespaceToken{});
    });

    etest::test("single quoted string with escaped code point", [] {
        auto output = run_tokenizer("'foo\\40'");
        expect_token(output, StringToken{"foo@"});
    });

    etest::test("ident token", [] {
        auto output = run_tokenizer("foo");

        expect_token(output, IdentToken{"foo"});
    });

    etest::test("ident token with digit", [] {
        auto output = run_tokenizer("f0o");

        expect_token(output, IdentToken{"f0o"});
    });

    etest::test("ident token starting with one dash", [] {
        auto output = run_tokenizer("-foo");

        expect_token(output, IdentToken{"-foo"});
    });

    etest::test("ident token starting with two dashes", [] {
        auto output = run_tokenizer("--foo");

        expect_token(output, IdentToken{"--foo"});
    });

    etest::test("ident token starting with underscore", [] {
        auto output = run_tokenizer("_foo-bar");

        expect_token(output, IdentToken{"_foo-bar"});
    });

    etest::test("ident token with escaped code point", [] {
        auto output = run_tokenizer("foo\\40");
        expect_token(output, IdentToken{"foo@"});
    });

    etest::test("ident token with escaped code point, eof", [] {
        auto output = run_tokenizer("foo\\");
        expect_token(output, IdentToken{"foo"s + kReplacementCharacter});
        expect_error(output, ParseError::EofInEscapeSequence);
    });

    etest::test("ident token with escaped code point, non-hex after", [] {
        auto output = run_tokenizer("foo\\40Z");
        expect_token(output, IdentToken{"foo@Z"});
    });

    etest::test("ident token with escaped code point, whitespace after", [] {
        auto output = run_tokenizer("foo\\40 ");
        expect_token(output, IdentToken{"foo@"});
    });

    etest::test("ident token with escaped code point, max characters in escape", [] {
        auto output = run_tokenizer("foo\\10fffff");
        // \u{10ffff} would've been nicer, but it's not yet supported by the compilers we support.
        expect_token(output, IdentToken{"foo\U0010FFFFf"});
    });

    etest::test("ident token with escaped code point, outside the unicode range", [] {
        auto output = run_tokenizer("foo\\110000");
        expect_token(output, IdentToken{"foo"s + kReplacementCharacter});
    });

    etest::test("ident token with escaped code point, surrogate", [] {
        auto output = run_tokenizer("foo\\d800");
        expect_token(output, IdentToken{"foo"s + kReplacementCharacter});
    });

    etest::test("ident token with escaped code point, null", [] {
        auto output = run_tokenizer("foo\\0");
        expect_token(output, IdentToken{"foo"s + kReplacementCharacter});
    });

    etest::test("whitespace after ident", [] {
        auto output = run_tokenizer("abc  ");

        expect_token(output, IdentToken{"abc"});
        expect_token(output, WhitespaceToken{});
    });

    etest::test("at keyword token", [] {
        auto output = run_tokenizer("@foo");

        expect_token(output, AtKeywordToken{"foo"});
    });

    etest::test("at keyword token with escaped code point", [] {
        auto output = run_tokenizer("@foo\\23");
        expect_token(output, AtKeywordToken{"foo#"});
    });

    etest::test("at keyword token with digit", [] {
        auto output = run_tokenizer("@b4z");

        expect_token(output, AtKeywordToken{"b4z"});
    });

    etest::test("at keyword token starting with one dash", [] {
        auto output = run_tokenizer("@-foo");

        expect_token(output, AtKeywordToken{"-foo"});
    });

    etest::test("at keyword token starting with two dashes", [] {
        auto output = run_tokenizer("@--foo");

        expect_token(output, AtKeywordToken{"--foo"});
    });

    etest::test("at keyword token starting with underscore", [] {
        auto output = run_tokenizer("@_foo-bar");

        expect_token(output, AtKeywordToken{"_foo-bar"});
    });

    etest::test("whitespace after at keyword token", [] {
        auto output = run_tokenizer("@foo ");

        expect_token(output, AtKeywordToken{"foo"});
        expect_token(output, WhitespaceToken{});
    });

    etest::test("at delimiter", [] {
        auto output = run_tokenizer("@ ");

        expect_token(output, DelimToken{'@'});
        expect_token(output, WhitespaceToken{});
    });

    etest::test("at dash delimiter", [] {
        auto output = run_tokenizer("@-");

        expect_token(output, DelimToken{'@'});
        expect_token(output, DelimToken{'-'});
    });

    etest::test("open paren", [] {
        auto output = run_tokenizer("(");

        expect_token(output, OpenParenToken{});
    });

    etest::test("close paren", [] {
        auto output = run_tokenizer(")");

        expect_token(output, CloseParenToken{});
    });

    etest::test("comma", [] {
        auto output = run_tokenizer(",");

        expect_token(output, CommaToken{});
    });

    etest::test("colon", [] {
        auto output = run_tokenizer(":");

        expect_token(output, ColonToken{});
    });

    etest::test("semicolon", [] {
        auto output = run_tokenizer(";");

        expect_token(output, SemiColonToken{});
    });

    etest::test("open square bracket", [] {
        auto output = run_tokenizer("[");

        expect_token(output, OpenSquareToken{});
    });

    etest::test("close square bracket", [] {
        auto output = run_tokenizer("]");

        expect_token(output, CloseSquareToken{});
    });

    etest::test("open curly bracket", [] {
        auto output = run_tokenizer("{");

        expect_token(output, OpenCurlyToken{});
    });

    etest::test("close curly bracket", [] {
        auto output = run_tokenizer("}");

        expect_token(output, CloseCurlyToken{});
    });

    etest::test("number: ez", [] {
        auto output = run_tokenizer("13");
        expect_token(output, NumberToken{.data = 13});
    });

    etest::test("number: leading 0", [] {
        auto output = run_tokenizer("00000001");
        expect_token(output, NumberToken{.data = 1});
    });

    etest::test("plus: number", [] {
        auto output = run_tokenizer("+13");
        expect_token(output, NumberToken{.data = 13});
    });

    etest::test("plus: number w/ leading 0", [] {
        auto output = run_tokenizer("+00000001");
        expect_token(output, NumberToken{.data = 1});
    });

    etest::test("plus: delim", [] {
        auto output = run_tokenizer("+hello");
        expect_token(output, DelimToken{'+'});
        expect_token(output, IdentToken{"hello"});
    });

    etest::test("hyphen: negative number", [] {
        auto output = run_tokenizer("-13");
        expect_token(output, NumberToken{.data = -13});
    });

    etest::test("hyphen: negative number w/ leading 0", [] {
        auto output = run_tokenizer("-00000001");
        expect_token(output, NumberToken{.data = -1});
    });

    etest::test("hyphen: cdc", [] {
        auto output = run_tokenizer("-->lol");
        expect_token(output, CdcToken{});
        expect_token(output, IdentToken{"lol"});
    });

    return etest::run_all_tests();
}
