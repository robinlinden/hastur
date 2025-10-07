// SPDX-FileCopyrightText: 2021-2025 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "css2/tokenizer.h"

#include "css2/token.h"

#include "etest/etest2.h"

#include <array>
#include <cstdint>
#include <limits>
#include <source_location>
#include <sstream>
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
        if (!tokens.empty()) {
            std::stringstream ss;
            ss << "Not all tokens were handled. Unhandled:\n";
            for (auto const &t : tokens) {
                ss << "* " << to_string(t) << '\n';
            }

            a.expectation_failure(ss.view(), loc);
        }

        if (!errors.empty()) {
            std::stringstream ss;
            ss << "Not all errors were handled. Unhandled:\n";
            for (auto e : errors) {
                ss << "* " << to_string(e) << '\n';
            }

            a.expectation_failure(ss.view(), loc);
        }
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
    Tokenizer{input, //
            [&](Token &&t) { tokens.push_back(std::move(t)); },
            [&](ParseError e) { errors.push_back(e); }}
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

    s.add_test("to_string(ParseError)", [](etest::IActions &a) {
        static constexpr auto kFirstError = ParseError::DisallowedCharacterInUrl;
        static constexpr auto kLastError = ParseError::NewlineInString;

        auto error = static_cast<int>(kFirstError);
        a.expect_eq(error, 0);

        while (error <= static_cast<int>(kLastError)) {
            a.expect(to_string(static_cast<ParseError>(error)) != "Unknown parse error",
                    std::to_string(error) + " is missing an error message");
            error += 1;
        }

        a.expect_eq(to_string(static_cast<ParseError>(error + 1)), "Unknown parse error");
    });

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

    s.add_test("escaped newline in string", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "'this is a\\\n blessed string'");
        expect_token(output, StringToken{"this is a blessed string"});
    });

    s.add_test("single quoted string with escaped code point", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "'foo\\40'");
        expect_token(output, StringToken{"foo@"});
    });

    s.add_test("string, escape before eof", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "'foo\\");
        expect_error(output, ParseError::EofInString);
        expect_token(output, StringToken{"foo"});
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

    s.add_test("at keyword starting w/ an escape", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "@\\23 bc");
        expect_token(output, AtKeywordToken{"#bc"});
    });

    s.add_test("at keyword starting w/ - + escape", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "@-\\23 bc");
        expect_token(output, AtKeywordToken{"-#bc"});
    });

    s.add_test("at keyword start, but with bad escape", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "@\\\n");
        expect_token(output, DelimToken{'@'});
        expect_error(output, ParseError::InvalidEscapeSequence);
        expect_token(output, DelimToken{'\\'});
        expect_token(output, WhitespaceToken{});
    });

    s.add_test("at keyword start, but with bad escape later", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "@aaa\\\n");
        expect_token(output, AtKeywordToken{"aaa"});
        expect_error(output, ParseError::InvalidEscapeSequence);
        expect_token(output, DelimToken{'\\'});
        expect_token(output, WhitespaceToken{});
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

    s.add_test("percentage: integer", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "13%");
        expect_token(output, PercentageToken{.data = 13});
    });

    s.add_test("percentage: large", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "12147483647%");
        expect_token(output, PercentageToken{std::numeric_limits<std::int32_t>::max()});
    });

    s.add_test("percentage: large negative", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "-12147483648%");
        expect_token(output, PercentageToken{std::numeric_limits<std::int32_t>::min()});
    });

    s.add_test("percentage: number", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "13.25%");
        expect_token(output, PercentageToken{.data = 13.25});
    });

    s.add_test("dimension", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "100lol@ 1.25em 5e\\23 ");
        expect_token(output, DimensionToken{.data = 100, .unit = "lol"});
        expect_token(output, DelimToken{'@'});
        expect_token(output, WhitespaceToken{});
        expect_token(output, DimensionToken{.data = 1.25, .unit = "em"});
        expect_token(output, WhitespaceToken{});
        expect_token(output, DimensionToken{.data = 5, .unit = "e#"});
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

    s.add_test("<: delim", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "<hello");
        expect_token(output, DelimToken{'<'});
        expect_token(output, IdentToken{"hello"});
    });

    s.add_test("<: cdo", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "<!--lol");
        expect_token(output, CdoToken{});
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

    s.add_test("number: e notation", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "1e3 1e+3 1e-3 1.0e3 1.0e+3 1.0e-3 5e0 -5e0 -3e2 -3e-2");
        expect_token(output, NumberToken{1000.});
        expect_token(output, WhitespaceToken{});
        expect_token(output, NumberToken{1000.});
        expect_token(output, WhitespaceToken{});
        expect_token(output, NumberToken{.001});
        expect_token(output, WhitespaceToken{});
        expect_token(output, NumberToken{1000.});
        expect_token(output, WhitespaceToken{});
        expect_token(output, NumberToken{1000.});
        expect_token(output, WhitespaceToken{});
        expect_token(output, NumberToken{.001});
        expect_token(output, WhitespaceToken{});
        expect_token(output, NumberToken{5.});
        expect_token(output, WhitespaceToken{});
        expect_token(output, NumberToken{-5.});
        expect_token(output, WhitespaceToken{});
        expect_token(output, NumberToken{-300.});
        expect_token(output, WhitespaceToken{});
        expect_token(output, NumberToken{-.03});
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

    s.add_test("hash token: ez", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "#");
        expect_token(output, DelimToken{'#'});
    });

    s.add_test("hash token: ident sequence", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "#foo");
        expect_token(output, HashToken{.type = HashToken::Type::Id, .data = "foo"});
    });

    s.add_test("hash token: non-ident sequence", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "#123");
        expect_token(output, HashToken{.type = HashToken::Type::Unrestricted, .data = "123"});
    });

    s.add_test("hash token: escaped code point", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "#\\41");
        expect_token(output, HashToken{.type = HashToken::Type::Id, .data = "A"});
    });

    s.add_test("hash token: invalid escape", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "#\\\n");
        expect_token(output, DelimToken{'#'});
        expect_error(output, ParseError::InvalidEscapeSequence);
        expect_token(output, DelimToken{'\\'});
        expect_token(output, WhitespaceToken{});
    });

    s.add_test("\\: ident-like", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "\\Hallo");
        expect_token(output, IdentToken{"Hallo"});
    });

    s.add_test("\\: invalid escape", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "\\\n");
        expect_error(output, ParseError::InvalidEscapeSequence);
        expect_token(output, DelimToken{'\\'});
        expect_token(output, WhitespaceToken{});
    });

    s.add_test("function: ez", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "foo()");
        expect_token(output, FunctionToken{.data = "foo"});
        expect_token(output, CloseParenToken{});
    });

    s.add_test("not a function", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "foo ()");
        expect_token(output, IdentToken{"foo"});
        expect_token(output, WhitespaceToken{});
        expect_token(output, OpenParenToken{});
        expect_token(output, CloseParenToken{});
    });

    s.add_test("function: url()-trickery", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "url(  'foo'  )");
        expect_token(output, FunctionToken{.data = "url"});
        expect_token(output, WhitespaceToken{});
        expect_token(output, StringToken{"foo"});
        expect_token(output, WhitespaceToken{});
        expect_token(output, CloseParenToken{});
    });

    s.add_test("function: more url()-trickery", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "url(\"foo\")");
        expect_token(output, FunctionToken{.data = "url"});
        expect_token(output, StringToken{"foo"});
        expect_token(output, CloseParenToken{});
    });

    s.add_test("url: obviously", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "url(foo)");
        expect_token(output, UrlToken{.data = "foo"});
    });

    s.add_test("url: eof", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "url( ");
        expect_error(output, ParseError::EofInUrl);
        expect_token(output, UrlToken{});
    });

    s.add_test("url: whitespace nonsense", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "url( test  \t\n)");
        expect_token(output, UrlToken{.data = "test"});
    });

    s.add_test("url: whitespace and eof", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "url( test  ");
        expect_error(output, ParseError::EofInUrl);
        expect_token(output, UrlToken{.data = "test"});
    });

    s.add_test("url: whitespace bad url", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "url( test  hello");
        expect_token(output, BadUrlToken{});
    });

    for (auto c : std::to_array<char>({'\'', '"', '\x08', '('})) {
        s.add_test("url: bad url: "s + c, [c](etest::IActions &a) {
            auto output = run_tokenizer(a, "url(hello"s + c);
            expect_error(output, ParseError::DisallowedCharacterInUrl);
            expect_token(output, BadUrlToken{});
        });
    }

    s.add_test("url: escape", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "url(\\41)");
        expect_token(output, UrlToken{.data = "A"});
    });

    s.add_test("url: BAD escape", [](etest::IActions &a) {
        auto output = run_tokenizer(a, "url(\\\n)");
        expect_error(output, ParseError::InvalidEscapeSequence);
        expect_token(output, BadUrlToken{});
    });

    return s.run();
}
