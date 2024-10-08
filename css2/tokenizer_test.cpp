// SPDX-FileCopyrightText: 2021-2024 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "css2/tokenizer.h"

#include "css2/token.h"

#include "etest/etest2.h"

#include <cstdint>
#include <limits>
#include <source_location>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace css2;
using namespace std::literals;

namespace {

constexpr char const *kReplacementCharacter = "\xef\xbf\xbd";

class TokenizerOutput {
public:
    ~TokenizerOutput() {
        a.expect(tokens.empty(), "Not all tokens were handled", loc);
        a.expect(errors.empty(), "Not all errors were handled", loc);
    }

    etest::IActions &a;
    std::vector<Token> tokens;
    std::vector<ParseError> errors;
    std::source_location loc;
};

TokenizerOutput run_tokenizer(
        etest::IActions &a, std::string_view input, std::source_location loc = std::source_location::current()) {
    std::vector<Token> tokens;
    std::vector<ParseError> errors;
    Tokenizer{input,
            [&](Token &&t) { tokens.push_back(std::move(t)); },
            [&](ParseError e) {
                errors.push_back(e);
            }}
            .run();
    return {a, std::move(tokens), std::move(errors), std::move(loc)};
}

void expect_token(
        TokenizerOutput &output, Token const &t, std::source_location const &loc = std::source_location::current()) {
    output.a.require(!output.tokens.empty(), "Unexpected end of token list", loc);
    output.a.expect_eq(output.tokens.front(), t, {}, loc);
    output.tokens.erase(begin(output.tokens));
}

void expect_error(
        TokenizerOutput &output, ParseError e, std::source_location const &loc = std::source_location::current()) {
    output.a.require(!output.errors.empty(), "Unexpected end of error list", loc);
    output.a.expect_eq(output.errors.front(), e, {}, loc);
    output.errors.erase(begin(output.errors));
}

} // namespace

int main() {
    etest::Suite s{};
    s.add_test("delimiter", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "?");

        expect_token(output, DelimToken{'?'});
    });

    s.add_test("comment", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "/* foo */");

        a.expect(output.tokens.empty());
    });

    s.add_test("comment with asterisks", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "/*****/");

        a.expect(output.tokens.empty());
    });

    s.add_test("comment almost started", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "/a");

        expect_token(output, DelimToken{'/'});
        expect_token(output, IdentToken{"a"});
    });

    s.add_test("delimiter after comment", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "/*/*/?");

        expect_token(output, DelimToken{'?'});
    });

    s.add_test("eof in comment", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "/* foo");

        expect_error(output, ParseError::EofInComment);
    });

    s.add_test("eof at comment ending", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "/* foo *");

        expect_error(output, ParseError::EofInComment);
    });

    s.add_test("space and comments", [](etest::IActions &a) {
        auto output = run_tokenizer(a, " /* */   /**/");

        expect_token(output, WhitespaceToken{});
        expect_token(output, WhitespaceToken{});
    });

    s.add_test("end with one tab", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "a\t");

        expect_token(output, IdentToken{"a"});
        expect_token(output, WhitespaceToken{});
    });

    s.add_test("end with two tabs", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "a\t\t");

        expect_token(output, IdentToken{"a"});
        expect_token(output, WhitespaceToken{});
    });

    s.add_test("end with one line feed", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "a\n");

        expect_token(output, IdentToken{"a"});
        expect_token(output, WhitespaceToken{});
    });

    s.add_test("end with two line feeds", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "a\n\n");

        expect_token(output, IdentToken{"a"});
        expect_token(output, WhitespaceToken{});
    });

    s.add_test("single quoted string", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "'this is a string'");

        expect_token(output, StringToken{"this is a string"});
    });

    s.add_test("double quoted string", [](etest::IActions &a) {
        auto output = run_tokenizer(a, R"("this is a string")");

        expect_token(output, StringToken{"this is a string"});
    });

    s.add_test("eof in string", [](etest::IActions &a) {
        auto output = run_tokenizer(a, R"("this is a)");

        expect_error(output, ParseError::EofInString);
        expect_token(output, StringToken{"this is a"});
    });

    s.add_test("newline in string", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "\"this is a\n");

        expect_error(output, ParseError::NewlineInString);
        expect_token(output, BadStringToken{});
        expect_token(output, WhitespaceToken{});
    });

    s.add_test("single quoted string with escaped code point", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "'foo\\40'");
        expect_token(output, StringToken{"foo@"});
    });

    s.add_test("ident token", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "foo");

        expect_token(output, IdentToken{"foo"});
    });

    s.add_test("ident token with digit", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "f0o");

        expect_token(output, IdentToken{"f0o"});
    });

    s.add_test("ident token starting with one dash", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "-foo");

        expect_token(output, IdentToken{"-foo"});
    });

    s.add_test("ident token starting with two dashes", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "--foo");

        expect_token(output, IdentToken{"--foo"});
    });

    s.add_test("ident token starting with underscore", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "_foo-bar");

        expect_token(output, IdentToken{"_foo-bar"});
    });

    s.add_test("ident token with escaped code point", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "foo\\40");
        expect_token(output, IdentToken{"foo@"});
    });

    s.add_test("ident token with escaped code point, eof", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "foo\\");
        expect_token(output, IdentToken{"foo"s + kReplacementCharacter});
        expect_error(output, ParseError::EofInEscapeSequence);
    });

    s.add_test("ident token with escaped code point, non-hex after", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "foo\\40Z");
        expect_token(output, IdentToken{"foo@Z"});
    });

    s.add_test("ident token with escaped code point, whitespace after", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "foo\\40 ");
        expect_token(output, IdentToken{"foo@"});
    });

    s.add_test("ident token with escaped code point, max characters in escape", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "foo\\10fffff");
        // \u{10ffff} would've been nicer, but it's not yet supported by the compilers we support.
        expect_token(output, IdentToken{"foo\U0010FFFFf"});
    });

    s.add_test("ident token with escaped code point, outside the unicode range", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "foo\\110000");
        expect_token(output, IdentToken{"foo"s + kReplacementCharacter});
    });

    s.add_test("ident token with escaped code point, surrogate", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "foo\\d800");
        expect_token(output, IdentToken{"foo"s + kReplacementCharacter});
    });

    s.add_test("ident token with escaped code point, null", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "foo\\0");
        expect_token(output, IdentToken{"foo"s + kReplacementCharacter});
    });

    s.add_test("whitespace after ident", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "abc  ");

        expect_token(output, IdentToken{"abc"});
        expect_token(output, WhitespaceToken{});
    });

    s.add_test("at keyword token", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "@foo");

        expect_token(output, AtKeywordToken{"foo"});
    });

    s.add_test("at keyword token with escaped code point", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "@foo\\23");
        expect_token(output, AtKeywordToken{"foo#"});
    });

    s.add_test("at keyword token with digit", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "@b4z");

        expect_token(output, AtKeywordToken{"b4z"});
    });

    s.add_test("at keyword token starting with one dash", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "@-foo");

        expect_token(output, AtKeywordToken{"-foo"});
    });

    s.add_test("at keyword token starting with two dashes", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "@--foo");

        expect_token(output, AtKeywordToken{"--foo"});
    });

    s.add_test("at keyword token starting with underscore", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "@_foo-bar");

        expect_token(output, AtKeywordToken{"_foo-bar"});
    });

    s.add_test("whitespace after at keyword token", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "@foo ");

        expect_token(output, AtKeywordToken{"foo"});
        expect_token(output, WhitespaceToken{});
    });

    s.add_test("at delimiter", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "@ ");

        expect_token(output, DelimToken{'@'});
        expect_token(output, WhitespaceToken{});
    });

    s.add_test("at dash delimiter", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "@-");

        expect_token(output, DelimToken{'@'});
        expect_token(output, DelimToken{'-'});
    });

    s.add_test("open paren", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "(");

        expect_token(output, OpenParenToken{});
    });

    s.add_test("close paren", [](etest::IActions &a) {
        auto output = run_tokenizer(a, ")");

        expect_token(output, CloseParenToken{});
    });

    s.add_test("comma", [](etest::IActions &a) {
        auto output = run_tokenizer(a, ",");

        expect_token(output, CommaToken{});
    });

    s.add_test("colon", [](etest::IActions &a) {
        auto output = run_tokenizer(a, ":");

        expect_token(output, ColonToken{});
    });

    s.add_test("semicolon", [](etest::IActions &a) {
        auto output = run_tokenizer(a, ";");

        expect_token(output, SemiColonToken{});
    });

    s.add_test("open square bracket", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "[");

        expect_token(output, OpenSquareToken{});
    });

    s.add_test("close square bracket", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "]");

        expect_token(output, CloseSquareToken{});
    });

    s.add_test("open curly bracket", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "{");

        expect_token(output, OpenCurlyToken{});
    });

    s.add_test("close curly bracket", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "}");

        expect_token(output, CloseCurlyToken{});
    });

    s.add_test("integer: ez", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "13");
        expect_token(output, NumberToken{.data = 13});
    });

    s.add_test("integer: less ez", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "(13)");
        expect_token(output, OpenParenToken{});
        expect_token(output, NumberToken{.data = 13});
        expect_token(output, CloseParenToken{});
    });

    s.add_test("integer: large", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "12147483647");
        expect_token(output, NumberToken{std::numeric_limits<std::int32_t>::max()});
    });

    s.add_test("integer: large negative", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "-12147483648");
        expect_token(output, NumberToken{std::numeric_limits<std::int32_t>::min()});
    });

    s.add_test("integer: leading 0", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "00000001");
        expect_token(output, NumberToken{.data = 1});
    });

    s.add_test("plus: integer", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "+13");
        expect_token(output, NumberToken{.data = 13});
    });

    s.add_test("plus: integer w/ leading 0", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "+00000001");
        expect_token(output, NumberToken{.data = 1});
    });

    s.add_test("plus: delim", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "+hello");
        expect_token(output, DelimToken{'+'});
        expect_token(output, IdentToken{"hello"});
    });

    s.add_test("hyphen: negative integer", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "-13");
        expect_token(output, NumberToken{.data = -13});
    });

    s.add_test("hyphen: negative integer w/ leading 0", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "-00000001");
        expect_token(output, NumberToken{.data = -1});
    });

    s.add_test("hyphen: cdc", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "-->lol");
        expect_token(output, CdcToken{});
        expect_token(output, IdentToken{"lol"});
    });

    s.add_test("number: ez", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "0.25");
        expect_token(output, NumberToken{0.25});
    });

    s.add_test("number: and other things", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "(0.375)");
        expect_token(output, OpenParenToken{});
        expect_token(output, NumberToken{0.375});
        expect_token(output, CloseParenToken{});
    });

    s.add_test("number: negative", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "-42.25");
        expect_token(output, NumberToken{-42.25});
    });

    s.add_test("number: with +", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "+13.25");
        expect_token(output, NumberToken{13.25});
    });

    s.add_test("number: negative, no digits before decimal point", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "-.25");
        expect_token(output, NumberToken{-.25});
    });

    s.add_test("number: with +, no digits before decimal point", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "+.25");
        expect_token(output, NumberToken{.25});
    });

    s.add_test("number: negative, abrupt end", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "-.");
        expect_token(output, DelimToken{'-'});
        expect_token(output, DelimToken{'.'});
    });

    s.add_test("number: large", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "12147483647.0");
        expect_token(output, NumberToken{static_cast<double>(std::numeric_limits<std::int32_t>::max())});
    });

    s.add_test("number: large negative", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "-12147483648.0");
        expect_token(output, NumberToken{static_cast<double>(std::numeric_limits<std::int32_t>::min())});
    });

    s.add_test("number: no digits before decimal point", [](etest::IActions &a) {
        auto output = run_tokenizer(a, ".25");
        expect_token(output, NumberToken{.25});
    });

    // TODO(robinlinden): Look into what this is meant to parse as.
    s.add_test("number: dots and digits shouldn't crash", [](etest::IActions &a) {
        auto output = run_tokenizer(a, ".25.25");
        expect_token(output, NumberToken{.25});
        expect_token(output, NumberToken{.25});
    });

    s.add_test("full stop", [](etest::IActions &a) {
        auto output = run_tokenizer(a, ".");
        expect_token(output, DelimToken{'.'});
    });

    s.add_test("full stop: extra junk", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "(.)");
        expect_token(output, OpenParenToken{});
        expect_token(output, DelimToken{'.'});
        expect_token(output, CloseParenToken{});
    });

    return s.run();
}
