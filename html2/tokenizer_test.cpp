// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "html2/tokenizer.h"

#include "etest/etest.h"

#include <array>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <fmt/format.h>

using namespace std::literals;

using etest::expect;
using etest::expect_eq;
using etest::require;

using namespace html2;

namespace {

static constexpr char const *kReplacementCharacter = "\xef\xbf\xbd";

class TokenizerOutput {
public:
    ~TokenizerOutput() {
        expect(tokens.empty(), "Not all tokens were handled", loc);
        expect(errors.empty(), "Not all errors were handled", loc);
    }

    std::vector<Token> tokens;
    std::vector<ParseError> errors;
    etest::source_location loc;
};

TokenizerOutput run_tokenizer(std::string_view input, etest::source_location loc = etest::source_location::current()) {
    std::vector<Token> tokens;
    std::vector<ParseError> errors;
    Tokenizer{input,
            [&](Tokenizer &tokenizer, Token &&t) {
                if (std::holds_alternative<StartTagToken>(t)) {
                    if (std::get<StartTagToken>(t).tag_name == "script") {
                        tokenizer.set_state(State::ScriptData);
                    }
                }
                tokens.push_back(std::move(t));
            },
            [&](auto &, ParseError e) {
                errors.push_back(e);
            }}
            .run();
    return {std::move(tokens), std::move(errors), std::move(loc)};
}

void expect_token(
        TokenizerOutput &output, Token t, etest::source_location const &loc = etest::source_location::current()) {
    require(!output.tokens.empty(), "Unexpected end of token list", loc);
    expect_eq(output.tokens.front(), t, {}, loc);
    output.tokens.erase(begin(output.tokens));
}

void expect_text(TokenizerOutput &output,
        std::string_view text,
        etest::source_location const &loc = etest::source_location::current()) {
    for (auto c : text) {
        expect_token(output, CharacterToken{c}, loc);
    }
}

void expect_error(
        TokenizerOutput &output, ParseError e, etest::source_location const &loc = etest::source_location::current()) {
    require(!output.errors.empty(), "Unexpected end of error list", loc);
    expect_eq(output.errors.front(), e, {}, loc);
    output.errors.erase(begin(output.errors));
}

void doctype_system_keyword_tests() {
    etest::test("doctype system keyword, single-quoted system identifier, missing space", [] {
        auto tokens = run_tokenizer("<!DOCTYPE HTML SYSTEM'great'>");
        expect_error(tokens, ParseError::MissingWhitespaceAfterDoctypeSystemKeyword);
        expect_token(tokens, DoctypeToken{.name = "html", .system_identifier = "great"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("doctype system keyword, double-quoted system identifier, missing space", [] {
        auto tokens = run_tokenizer(R"(<!DOCTYPE HTML SYSTEM"great">)");
        expect_error(tokens, ParseError::MissingWhitespaceAfterDoctypeSystemKeyword);
        expect_token(tokens, DoctypeToken{.name = "html", .system_identifier = "great"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("doctype system keyword, missing identifier", [] {
        auto tokens = run_tokenizer(R"(<!DOCTYPE HTML SYSTEM>)");
        expect_error(tokens, ParseError::MissingDoctypeSystemIdentifier);
        expect_token(tokens, DoctypeToken{.name = "html", .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("doctype system keyword, missing quote before identifier", [] {
        auto tokens = run_tokenizer(R"(<!DOCTYPE HTML SYSTEMgreat>)");
        expect_error(tokens, ParseError::MissingQuoteBeforeDoctypeSystemIdentifier);
        expect_token(tokens, DoctypeToken{.name = "html", .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("doctype system keyword, eof in doctype", [] {
        auto tokens = run_tokenizer(R"(<!DOCTYPE HTML SYSTEM)");
        expect_error(tokens, ParseError::EofInDoctype);
        expect_token(tokens, DoctypeToken{.name = "html", .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("doctype before system identifier, single-quoted system identifier", [] {
        auto tokens = run_tokenizer("<!DOCTYPE HTML SYSTEM 'great'>");
        expect_token(tokens, DoctypeToken{.name = "html", .system_identifier = "great"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("doctype before system identifier, double-quoted system identifier", [] {
        auto tokens = run_tokenizer(R"(<!DOCTYPE HTML SYSTEM "great">)");
        expect_token(tokens, DoctypeToken{.name = "html", .system_identifier = "great"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("doctype before system identifier, more eof in doctype", [] {
        auto tokens = run_tokenizer(R"(<!DOCTYPE HTML SYSTEM   )");
        expect_error(tokens, ParseError::EofInDoctype);
        expect_token(tokens, DoctypeToken{.name = "html", .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("doctype before system identifier, missing identifier", [] {
        auto tokens = run_tokenizer(R"(<!DOCTYPE HTML SYSTEM >)");
        expect_error(tokens, ParseError::MissingDoctypeSystemIdentifier);
        expect_token(tokens, DoctypeToken{.name = "html", .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("doctype before system identifier, missing quote before identifier", [] {
        auto tokens = run_tokenizer(R"(<!DOCTYPE HTML SYSTEM great>)");
        expect_error(tokens, ParseError::MissingQuoteBeforeDoctypeSystemIdentifier);
        expect_token(tokens, DoctypeToken{.name = "html", .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });
}

} // namespace

int main() {
    doctype_system_keyword_tests();

    etest::test("script, empty", [] {
        auto tokens = run_tokenizer("<script></script>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("script, upper case tag", [] {
        auto tokens = run_tokenizer("<SCRIPT></SCRIPT>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("script, with code", [] {
        auto tokens = run_tokenizer("<script>code</script>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "code"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("script, unexpected null", [] {
        auto tokens = run_tokenizer("<script>\0</script>"sv);

        expect_error(tokens, ParseError::UnexpectedNullCharacter);
        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, kReplacementCharacter);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("script, with source file attribute", [] {
        auto tokens = run_tokenizer("<script src=\"/foo.js\"></script>");

        expect_token(tokens, StartTagToken{.tag_name = "script", .attributes = {{"src", "/foo.js"}}});
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("script, end tag as text", [] {
        auto tokens = run_tokenizer("<script></</script>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "</"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("script, misspelled end tag", [] {
        auto tokens = run_tokenizer("<script></scropt>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "</scropt>"sv);
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("script, almost escaped", [] {
        auto tokens = run_tokenizer("<script><!</script>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("script, almost escaped dash", [] {
        auto tokens = run_tokenizer("<script><!-<</script>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!-<"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("script, escaped", [] {
        auto tokens = run_tokenizer("<script><!-- </script> --></script>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!-- "sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_text(tokens, " -->"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("script, escaped null", [] {
        auto tokens = run_tokenizer("<script><!-- \0 --></script>"sv);

        expect_error(tokens, ParseError::UnexpectedNullCharacter);
        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!-- "s + kReplacementCharacter + " -->");
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("script, escaped one dash", [] {
        auto tokens = run_tokenizer("<script><!-- -<</script>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!-- -<"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("script, escaped dash null", [] {
        auto tokens = run_tokenizer("<script><!-- -\0</script>"sv);

        expect_error(tokens, ParseError::UnexpectedNullCharacter);
        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!-- -"s + kReplacementCharacter);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("script, escaped dash dash null", [] {
        auto tokens = run_tokenizer("<script><!-- --\0</script>"sv);

        expect_error(tokens, ParseError::UnexpectedNullCharacter);
        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!-- --"s + kReplacementCharacter);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("script, escaped one dash and back to escaped", [] {
        auto tokens = run_tokenizer("<script><!-- -x</script>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!-- -x"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("script, escaped upper case", [] {
        auto tokens = run_tokenizer("<script><!--- </SCRIPT> ---></script>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!--- "sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_text(tokens, " --->"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("script, escaped dummy tags", [] {
        auto tokens = run_tokenizer("<script><!-- <</xyz>> --></script>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!-- <</xyz>> -->"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("script, double escaped", [] {
        auto tokens = run_tokenizer("<script><!--<script>code</script>--></script>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!--<script>code</script>-->"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("script, double escaped null", [] {
        auto tokens = run_tokenizer("<script><!--<script>\0</script>--></script>"sv);

        expect_error(tokens, ParseError::UnexpectedNullCharacter);
        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!--<script>"s + kReplacementCharacter + "</script>-->");
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("script, double escaped dash", [] {
        auto tokens = run_tokenizer("<script><!--<script>---</script>--></script>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!--<script>---</script>-->"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("script, double escaped dash null", [] {
        auto tokens = run_tokenizer("<script><!--<script>-\0</script>--></script>"sv);

        expect_error(tokens, ParseError::UnexpectedNullCharacter);
        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!--<script>-"s + kReplacementCharacter + "</script>-->");
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("script, double escaped dash dash null", [] {
        auto tokens = run_tokenizer("<script><!--<script>--\0</script>--></script>"sv);

        expect_error(tokens, ParseError::UnexpectedNullCharacter);
        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!--<script>--"s + kReplacementCharacter + "</script>-->");
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("script, double escaped less than", [] {
        auto tokens = run_tokenizer("<script><!--<script><</xyz>></script>--></script>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!--<script><</xyz>></script>-->"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("script, double escaped dash less than", [] {
        auto tokens = run_tokenizer("<SCRIPT><!--<SCRIPT>-<</SCRIPT>--></SCRIPT>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!--<SCRIPT>-<</SCRIPT>-->"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("script, double escaped dash less than", [] {
        auto tokens = run_tokenizer("<SCRIPT><!--<SCRIPT>-->--></SCRIPT>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!--<SCRIPT>-->-->"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("script, end tag with attribute", [] {
        auto tokens = run_tokenizer("<script></script src=\"/foo.js\">");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_token(tokens, EndTagToken{.tag_name = "script", .attributes = {{"src", "/foo.js"}}});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("script, misspelled end tag with attribute", [] {
        auto tokens = run_tokenizer("<script></scropt src=\"/foo.js\">");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "</scropt src=\"/foo.js\">"sv);
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("script, self closing end tag", [] {
        auto tokens = run_tokenizer("<script></script/>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_token(tokens, EndTagToken{.tag_name = "script", .self_closing = true});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("script, misspelled self closing end tag", [] {
        auto tokens = run_tokenizer("<script></scropt/>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "</scropt/>"sv);
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("script, escaped end tag open", [] {
        auto tokens = run_tokenizer("<script><!--</>--></script>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!--</>-->"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("script, escaped end tag with attributes", [] {
        auto tokens = run_tokenizer("<script><!--</script src=\"/bar.js\">--></script>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!--"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script", .attributes = {{"src", "/bar.js"}}});
        expect_text(tokens, "-->"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("script, misspelled escaped end tag with attributes", [] {
        auto tokens = run_tokenizer("<script><!--</scropt src=\"/bar.js\">--></script>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!--</scropt src=\"/bar.js\">-->"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("script, escaped self closing end tag", [] {
        auto tokens = run_tokenizer("<script><!--</script/>--></script>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!--"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script", .self_closing = true});
        expect_text(tokens, "-->"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("script, misspelled escaped self closing end tag", [] {
        auto tokens = run_tokenizer("<script><!--</scropt/>--></script>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!--</scropt/>-->"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("script, eof in less than sign", [] {
        auto tokens = run_tokenizer("<script><");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<"sv);
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("script, eof in end tag open", [] {
        auto tokens = run_tokenizer("<script></scr");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "</scr"sv);
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("comment, simple", [] {
        auto tokens = run_tokenizer("<!-- Hello -->");
        expect_token(tokens, CommentToken{.data = " Hello "});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("comment, empty", [] {
        auto tokens = run_tokenizer("<!---->");
        expect_token(tokens, CommentToken{.data = ""});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("comment, with dashes and bang", [] {
        auto tokens = run_tokenizer("<!--!-->");
        expect_token(tokens, CommentToken{.data = "!"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("comment, with new lines", [] {
        auto tokens = run_tokenizer("<!--\nOne\nTwo\n-->");
        expect_token(tokens, CommentToken{.data = "\nOne\nTwo\n"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("comment, multiple with new lines", [] {
        auto tokens = run_tokenizer("<!--a-->\n<!--b-->\n<!--c-->");
        expect_token(tokens, CommentToken{.data = "a"});
        expect_token(tokens, CharacterToken{'\n'});
        expect_token(tokens, CommentToken{.data = "b"});
        expect_token(tokens, CharacterToken{'\n'});
        expect_token(tokens, CommentToken{.data = "c"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("comment, allowed to end with <!", [] {
        auto tokens = run_tokenizer("<!--My favorite operators are > and <!-->");
        expect_token(tokens, CommentToken{.data = "My favorite operators are > and <!"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("comment, nested comment", [] {
        auto tokens = run_tokenizer("<!--<!---->");
        expect_error(tokens, ParseError::NestedComment);
        expect_token(tokens, CommentToken{.data = "<!--"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("comment, nested comment closed", [] {
        auto tokens = run_tokenizer("<!-- <!-- nested --> -->");
        expect_error(tokens, ParseError::NestedComment);
        expect_token(tokens, CommentToken{.data = " <!-- nested "});
        expect_text(tokens, " -->");
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("comment, abrupt closing in comment start", [] {
        auto tokens = run_tokenizer("<!-->");
        expect_error(tokens, ParseError::AbruptClosingOfEmptyComment);
        expect_token(tokens, CommentToken{.data = ""});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("comment, abrupt closing in comment start dash", [] {
        auto tokens = run_tokenizer("<!--->");
        expect_error(tokens, ParseError::AbruptClosingOfEmptyComment);
        expect_token(tokens, CommentToken{.data = ""});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("comment, incorrectly closed comment", [] {
        auto tokens = run_tokenizer("<!--abc--!>");
        expect_error(tokens, ParseError::IncorrectlyClosedComment);
        expect_token(tokens, CommentToken{.data = "abc"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("comment, end before comment", [] {
        auto tokens = run_tokenizer("<!--");
        expect_error(tokens, ParseError::EofInComment);
        expect_token(tokens, CommentToken{.data = ""});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("comment, eof before comment is closed", [] {
        auto tokens = run_tokenizer("<!--abc");
        expect_error(tokens, ParseError::EofInComment);
        expect_token(tokens, CommentToken{.data = "abc"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("character entity reference, simple", [] {
        auto tokens = run_tokenizer("&lt;");
        expect_token(tokens, CharacterToken{'<'});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("character entity reference, only &", [] {
        auto tokens = run_tokenizer("&");
        expect_token(tokens, CharacterToken{'&'});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("character entity reference, not ascii alphanumeric", [] {
        auto tokens = run_tokenizer("&@");
        expect_text(tokens, "&@");
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("character entity reference, reference to non-ascii glyph", [] {
        auto tokens = run_tokenizer("&div;");
        expect_text(tokens, "\xc3\xb7"sv);
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("character entity reference, two unicode code points required", [] {
        auto tokens = run_tokenizer("&acE;");
        expect_text(tokens, "\xe2\x88\xbe\xcc\xb3"sv);
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("ambiguous ampersand", [] {
        auto tokens = run_tokenizer("&blah;");
        expect_text(tokens, "&blah;");
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("ambiguous ampersand in attribute", [] {
        auto tokens = run_tokenizer("<p attr='&blah;'>");
        expect_token(tokens, StartTagToken{.tag_name = "p", .attributes = {{"attr", "&blah;"}}});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("attribute, one attribute single quoted", [] {
        auto tokens = run_tokenizer("<tag a='b'>");

        expect_token(tokens, StartTagToken{.tag_name = "tag", .attributes = {{"a", "b"}}});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("attribute, one attribute double quoted", [] {
        auto tokens = run_tokenizer("<tag a=\"b\">");

        expect_token(tokens, StartTagToken{.tag_name = "tag", .attributes = {{"a", "b"}}});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("attribute, one uppercase attribute", [] {
        auto tokens = run_tokenizer("<tag ATTRIB=\"ABC123\">");

        expect_token(tokens, StartTagToken{.tag_name = "tag", .attributes = {{"attrib", "ABC123"}}});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("attribute, multiple attributes", [] {
        auto tokens = run_tokenizer("<tag  foo=\"bar\" A='B'  value='321'>");

        expect_token(
                tokens, StartTagToken{.tag_name = "tag", .attributes = {{"foo", "bar"}, {"a", "B"}, {"value", "321"}}});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("attribute, one attribute unquoted", [] {
        auto tokens = run_tokenizer("<tag a=b>");
        expect_token(tokens, StartTagToken{.tag_name = "tag", .attributes = {{"a", "b"}}});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("attribute, multiple attributes unquoted", [] {
        auto tokens = run_tokenizer("<tag a=b c=d>");
        expect_token(tokens, StartTagToken{.tag_name = "tag", .attributes = {{"a", "b"}, {"c", "d"}}});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("attribute, multiple attributes unquoted", [] {
        auto tokens = run_tokenizer("<tag a=b c=d>");
        expect_token(tokens, StartTagToken{.tag_name = "tag", .attributes = {{"a", "b"}, {"c", "d"}}});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("attribute, unexpected-character-in-unquoted-attribute", [] {
        auto tokens = run_tokenizer("<tag a=b=c>");
        expect_error(tokens, ParseError::UnexpectedCharacterInUnquotedAttributeValue);
        expect_token(tokens, StartTagToken{.tag_name = "tag", .attributes = {{"a", "b=c"}}});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("attribute, unquoted, eof-in-tag", [] {
        auto tokens = run_tokenizer("<tag a=b");
        expect_error(tokens, ParseError::EofInTag);
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("attribute, unquoted, with character reference", [] {
        auto tokens = run_tokenizer("<tag a=&amp>");
        expect_token(tokens, StartTagToken{.tag_name = "tag", .attributes = {{"a", "&"}}});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("attribute, unquoted, unexpected-null-character", [] {
        auto tokens = run_tokenizer("<tag a=\0>"sv);
        expect_error(tokens, ParseError::UnexpectedNullCharacter);
        expect_token(tokens, StartTagToken{.tag_name = "tag", .attributes = {{"a", kReplacementCharacter}}});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("numeric character reference", [] {
        auto tokens = run_tokenizer("&#9731;"); // U+2603: SNOWMAN
        expect_text(tokens, "\xe2\x98\x83");
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("hexadecimal character reference", [] {
        auto tokens = run_tokenizer("&#x2721;"); // U+2721
        expect_text(tokens, "\xe2\x9c\xa1");
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("hexadecimal character reference, upper hex digits", [] {
        auto tokens = run_tokenizer("&#x27FF;"); // U+27FF
        expect_text(tokens, "\xe2\x9f\xbf");
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("hexadecimal character reference, lower hex digits", [] {
        auto tokens = run_tokenizer("&#x27ff;"); // U+27FF
        expect_text(tokens, "\xe2\x9f\xbf");
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("hexadecimal character reference, no semicolon", [] {
        auto tokens = run_tokenizer("&#x27ff "); // U+27FF
        expect_error(tokens, ParseError::MissingSemicolonAfterCharacterReference);
        expect_text(tokens, "\xe2\x9f\xbf "); // Note the bonus space.
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("hexadecimal character reference, abrupt end", [] {
        auto tokens = run_tokenizer("&#x27ff"); // U+27FF
        expect_error(tokens, ParseError::MissingSemicolonAfterCharacterReference);
        expect_text(tokens, "\xe2\x9f\xbf");
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("hexadecimal character reference, no digits", [] {
        auto tokens = run_tokenizer("&#xG;");
        expect_error(tokens, ParseError::AbsenceOfDigitsInNumericCharacterReference);
        expect_text(tokens, "&#xG;");
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("character reference, c0 control character", [] {
        auto tokens = run_tokenizer("&#x01;");
        expect_error(tokens, ParseError::ControlCharacterReference);
        expect_text(tokens, "\x01");
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("doctype, eof after name", [] {
        auto tokens = run_tokenizer("<!doctype html ");
        expect_error(tokens, ParseError::EofInDoctype);
        expect_token(tokens, DoctypeToken{.name = "html", .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("doctype, closing tag after whitespace", [] {
        auto tokens = run_tokenizer("<!doctype html  >");
        expect_token(tokens, DoctypeToken{.name = "html"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("doctype, bogus doctype", [] {
        auto tokens = run_tokenizer("<!doctype html bogus>");
        expect_error(tokens, ParseError::InvalidCharacterSequenceAfterDoctypeName);
        expect_token(tokens, DoctypeToken{.name = "html", .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("doctype, bogus doctype, null character and eof", [] {
        auto tokens = run_tokenizer("<!doctype html b\0gus"sv);
        expect_error(tokens, ParseError::InvalidCharacterSequenceAfterDoctypeName);
        expect_error(tokens, ParseError::UnexpectedNullCharacter);
        expect_token(tokens, DoctypeToken{.name = "html", .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    for (char quote : std::array{'\'', '"'}) {
        auto type = quote == '"' ? "double"sv : "single"sv;

        etest::test(fmt::format("doctype, {}-quoted public identifier", type), [=] {
            auto tokens = run_tokenizer(fmt::format("<!DOCTYPE HTML PUBLIC {0}great{0}>", quote));
            expect_token(tokens, DoctypeToken{.name = "html", .public_identifier = "great"});
            expect_token(tokens, EndOfFileToken{});
        });

        etest::test(fmt::format("doctype, {}-quoted public identifier, missing whitespace", type), [=] {
            auto tokens = run_tokenizer(fmt::format("<!DOCTYPE HTML PUBLIC{0}great{0}>", quote));
            expect_error(tokens, ParseError::MissingWhitespaceAfterDoctypePublicKeyword);
            expect_token(tokens, DoctypeToken{.name = "html", .public_identifier = "great"});
            expect_token(tokens, EndOfFileToken{});
        });

        etest::test(fmt::format("doctype, {}-quoted public identifier, eof", type), [=] {
            auto tokens = run_tokenizer(fmt::format("<!DOCTYPE HTML PUBLIC {0}great", quote));
            expect_error(tokens, ParseError::EofInDoctype);
            expect_token(tokens, DoctypeToken{.name = "html", .public_identifier = "great", .force_quirks = true});
            expect_token(tokens, EndOfFileToken{});
        });

        etest::test(fmt::format("doctype, {}-quoted public identifier, abrupt end", type), [=] {
            auto tokens = run_tokenizer(fmt::format("<!DOCTYPE HTML PUBLIC {0}great>", quote));
            expect_error(tokens, ParseError::AbruptDoctypePublicIdentifier);
            expect_token(tokens, DoctypeToken{.name = "html", .public_identifier = "great", .force_quirks = true});
            expect_token(tokens, EndOfFileToken{});
        });

        etest::test(fmt::format("doctype, {}-quoted public identifier, null", type), [=] {
            auto tokens = run_tokenizer(fmt::format("<!DOCTYPE HTML PUBLIC {0}gre\0t{0}>"sv, quote));
            expect_error(tokens, ParseError::UnexpectedNullCharacter);
            expect_token(
                    tokens, DoctypeToken{.name = "html", .public_identifier = "gre"s + kReplacementCharacter + "t"});
            expect_token(tokens, EndOfFileToken{});
        });

        etest::test(fmt::format("doctype, {}-quoted system identifier", type), [=] {
            auto tokens = run_tokenizer(fmt::format("<!DOCTYPE HTML PUBLIC 'great' {0}hello{0}>", quote));
            expect_token(
                    tokens, DoctypeToken{.name = "html", .public_identifier = "great", .system_identifier = "hello"});
            expect_token(tokens, EndOfFileToken{});
        });

        etest::test(fmt::format("doctype, {}-quoted system identifier, unexpected null", type), [=] {
            auto tokens = run_tokenizer(fmt::format("<!DOCTYPE HTML PUBLIC 'great' {0}n\0{0}>"sv, quote));
            expect_error(tokens, ParseError::UnexpectedNullCharacter);
            expect_token(tokens,
                    DoctypeToken{.name = "html",
                            .public_identifier = "great",
                            .system_identifier = "n"s + kReplacementCharacter});
            expect_token(tokens, EndOfFileToken{});
        });

        etest::test(fmt::format("doctype, {}-quoted system identifier, missing whitespace", type), [=] {
            auto tokens = run_tokenizer(fmt::format("<!DOCTYPE HTML PUBLIC 'great'{0}hello{0}>", quote));
            expect_error(tokens, ParseError::MissingWhitespaceBetweenDoctypePublicAndSystemIdentifiers);
            expect_token(
                    tokens, DoctypeToken{.name = "html", .public_identifier = "great", .system_identifier = "hello"});
            expect_token(tokens, EndOfFileToken{});
        });

        etest::test(fmt::format("doctype, {}-quoted system identifier, eof", type), [=] {
            auto tokens = run_tokenizer(fmt::format("<!DOCTYPE HTML PUBLIC 'great' {0}hell", quote));
            expect_error(tokens, ParseError::EofInDoctype);
            expect_token(tokens,
                    DoctypeToken{.name = "html",
                            .public_identifier = "great",
                            .system_identifier = "hell",
                            .force_quirks = true});
            expect_token(tokens, EndOfFileToken{});
        });

        etest::test(fmt::format("doctype, {}-quoted system identifier, abrupt end", type), [=] {
            auto tokens = run_tokenizer(fmt::format("<!DOCTYPE HTML PUBLIC 'great' {0}hell>", quote));
            expect_error(tokens, ParseError::AbruptDoctypeSystemIdentifier);
            expect_token(tokens,
                    DoctypeToken{.name = "html",
                            .public_identifier = "great",
                            .system_identifier = "hell",
                            .force_quirks = true});
            expect_token(tokens, EndOfFileToken{});
        });
    }

    etest::test("doctype, system identifier, missing quote", [=] {
        auto tokens = run_tokenizer(R"(<!DOCTYPE HTML PUBLIC "great" hello>)");
        expect_error(tokens, ParseError::MissingQuoteBeforeDoctypeSystemIdentifier);
        expect_token(tokens, DoctypeToken{.name = "html", .public_identifier = "great", .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("doctype, after system identifier, eof", [=] {
        auto tokens = run_tokenizer(R"(<!DOCTYPE HTML PUBLIC "great" "hello" )");
        expect_error(tokens, ParseError::EofInDoctype);
        expect_token(tokens,
                DoctypeToken{.name = "html",
                        .public_identifier = "great",
                        .system_identifier = "hello",
                        .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("doctype, after system identifier, unexpected character", [=] {
        auto tokens = run_tokenizer(R"(<!DOCTYPE HTML PUBLIC "great" "hello" ohno>)");
        expect_error(tokens, ParseError::UnexpectedCharacterAfterDoctypeSystemIdentifier);
        expect_token(tokens, DoctypeToken{.name = "html", .public_identifier = "great", .system_identifier = "hello"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("doctype, between public and system identifiers, eof", [=] {
        auto tokens = run_tokenizer(R"(<!DOCTYPE HTML PUBLIC "great"  )");
        expect_error(tokens, ParseError::EofInDoctype);
        expect_token(tokens, DoctypeToken{.name = "html", .public_identifier = "great", .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("doctype, between public and system identifiers", [=] {
        auto tokens = run_tokenizer(R"(<!DOCTYPE HTML PUBLIC "great" >)");
        expect_token(tokens, DoctypeToken{.name = "html", .public_identifier = "great"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("doctype, public identifier, missing quotes", [] {
        auto tokens = run_tokenizer("<!DOCTYPE HTML PUBLIC great>");
        expect_error(tokens, ParseError::MissingQuoteBeforeDoctypePublicIdentifier);
        expect_token(tokens, DoctypeToken{.name = "html", .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("doctype, public identifier, no space", [] {
        auto tokens = run_tokenizer("<!DOCTYPE HTML PUBLICgreat>");
        expect_error(tokens, ParseError::MissingQuoteBeforeDoctypePublicIdentifier);
        expect_token(tokens, DoctypeToken{.name = "html", .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("doctype, public identifier, no space", [] {
        auto tokens = run_tokenizer(R"(<!DOCTYPE HTML PUBLIC "great"bad>)");
        expect_error(tokens, ParseError::MissingQuoteBeforeDoctypeSystemIdentifier);
        expect_token(tokens, DoctypeToken{.name = "html", .public_identifier = "great", .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("doctype, public keyword, eof", [] {
        auto tokens = run_tokenizer("<!DOCTYPE HTML PUBLIC");
        expect_error(tokens, ParseError::EofInDoctype);
        expect_token(tokens, DoctypeToken{.name = "html", .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("doctype, public keyword, missing identifier", [] {
        auto tokens = run_tokenizer("<!DOCTYPE HTML PUBLIC>");
        expect_error(tokens, ParseError::MissingDoctypePublicIdentifier);
        expect_token(tokens, DoctypeToken{.name = "html", .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("doctype, after public keyword, eof", [] {
        auto tokens = run_tokenizer("<!DOCTYPE HTML PUBLIC  ");
        expect_error(tokens, ParseError::EofInDoctype);
        expect_token(tokens, DoctypeToken{.name = "html", .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("doctype, public keyword but no identifier", [] {
        auto tokens = run_tokenizer("<!DOCTYPE HTML PUBLIC >");
        expect_error(tokens, ParseError::MissingDoctypePublicIdentifier);
        expect_token(tokens, DoctypeToken{.name = "html", .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("doctype, eof after public identifier", [] {
        auto tokens = run_tokenizer(R"(<!DOCTYPE HTML PUBLIC "great")");
        expect_error(tokens, ParseError::EofInDoctype);
        expect_token(tokens, DoctypeToken{.name = "html", .public_identifier = "great", .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("tag closed after attribute name", [] {
        auto tokens = run_tokenizer("<one a><two b>");
        expect_token(tokens, StartTagToken{.tag_name = "one", .attributes = {{"a", ""}}});
        expect_token(tokens, StartTagToken{.tag_name = "two", .attributes = {{"b", ""}}});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("pages served as xml don't break everything", [] {
        auto tokens = run_tokenizer("<?xml?><!DOCTYPE HTML>");
        expect_error(tokens, ParseError::InvalidFirstCharacterOfTagName);
        expect_text(tokens, "<?xml?>");
        expect_token(tokens, DoctypeToken{.name = "html"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("invalid end tag open, eof", [] {
        auto tokens = run_tokenizer("</!bogus");
        expect_error(tokens, ParseError::InvalidFirstCharacterOfTagName);
        expect_token(tokens, CommentToken{.data = "!bogus"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("invalid end tag open, unexpected null", [] {
        auto tokens = run_tokenizer("</!bogu\0>"sv);
        expect_error(tokens, ParseError::InvalidFirstCharacterOfTagName);
        expect_error(tokens, ParseError::UnexpectedNullCharacter);
        expect_token(tokens, CommentToken{.data = "!bogu"s + kReplacementCharacter});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("tag name, unexpected null", [] {
        auto tokens = run_tokenizer("<hell\0>"sv);
        expect_error(tokens, ParseError::UnexpectedNullCharacter);
        expect_token(tokens, StartTagToken{.tag_name{"hell"s + kReplacementCharacter}});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("attribute name, unexpected null", [] {
        auto tokens = run_tokenizer("<hello a\0>"sv);
        expect_error(tokens, ParseError::UnexpectedNullCharacter);
        expect_token(tokens, StartTagToken{.tag_name{"hello"s}, .attributes{{"a"s + kReplacementCharacter, ""}}});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("attribute value, unexpected null", [] {
        for (auto html : {"<a b=\"\0\">"sv, "<a b='\0'>"sv}) {
            auto tokens = run_tokenizer(html);
            expect_error(tokens, ParseError::UnexpectedNullCharacter);
            expect_token(tokens, StartTagToken{.tag_name{"a"s}, .attributes{{"b"s, kReplacementCharacter}}});
            expect_token(tokens, EndOfFileToken{});
        }
    });

    etest::test("comment, unexpected null", [] {
        auto tokens = run_tokenizer("<!--\0-->"sv);
        expect_error(tokens, ParseError::UnexpectedNullCharacter);
        expect_token(tokens, CommentToken{.data{kReplacementCharacter}});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("before doctype name, unexpected null", [] {
        auto tokens = run_tokenizer("<!doctype \0hi>"sv);
        expect_error(tokens, ParseError::UnexpectedNullCharacter);
        expect_token(tokens, DoctypeToken{.name{kReplacementCharacter + "hi"s}});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("doctype name, unexpected null", [] {
        auto tokens = run_tokenizer("<!doctype hi\0>"sv);
        expect_error(tokens, ParseError::UnexpectedNullCharacter);
        expect_token(tokens, DoctypeToken{.name{"hi"s + kReplacementCharacter}});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("doctype, eof", [] {
        auto tokens = run_tokenizer("<!doctype"sv);
        expect_error(tokens, ParseError::EofInDoctype);
        expect_token(tokens, DoctypeToken{.force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("doctype, missing doctype name", [] {
        auto tokens = run_tokenizer("<!doctype>"sv);
        expect_error(tokens, ParseError::MissingDoctypeName);
        expect_token(tokens, DoctypeToken{.force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("doctype, missing whitespace before doctype name", [] {
        auto tokens = run_tokenizer("<!doctypelol>"sv);
        expect_error(tokens, ParseError::MissingWhitespaceBeforeDoctypeName);
        expect_token(tokens, DoctypeToken{.name = "lol"});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("before doctype name, eof", [] {
        auto tokens = run_tokenizer("<!doctype "sv);
        expect_error(tokens, ParseError::EofInDoctype);
        expect_token(tokens, DoctypeToken{.force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("doctype name, eof", [] {
        auto tokens = run_tokenizer("<!doctype hi"sv);
        expect_error(tokens, ParseError::EofInDoctype);
        expect_token(tokens, DoctypeToken{.name = "hi", .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    return etest::run_all_tests();
}
