// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "html2/tokenizer.h"

#include "etest/etest.h"

#include <fstream>
#include <iterator>
#include <string>
#include <utility>
#include <variant>
#include <vector>

using namespace std::literals;

using etest::expect;
using etest::expect_eq;
using etest::require;

using namespace html2;

namespace {

static constexpr char const *kReplacementCharacter = "\xef\xbf\xbd";

std::vector<Token> run_tokenizer(std::string_view input) {
    std::vector<Token> tokens;
    Tokenizer{input,
            [&](Tokenizer &tokenizer, Token &&t) {
                if (std::holds_alternative<StartTagToken>(t)) {
                    if (std::get<StartTagToken>(t).tag_name == "script") {
                        tokenizer.set_state(State::ScriptData);
                    }
                }
                tokens.push_back(std::move(t));
            }}
            .run();
    return tokens;
}

void expect_token(std::vector<Token> &tokens, Token t) {
    expect_eq(tokens.front(), t);
    tokens.erase(begin(tokens));
}

void expect_text(std::vector<Token> &tokens, std::string_view text) {
    for (auto c : text) {
        expect_token(tokens, CharacterToken{c});
    }
}

} // namespace

int main() {
    etest::test("simple_page", [] {
        std::ifstream page{"html2/test/simple_page.html", std::ios::binary};
        require(page.is_open());
        std::string page_str{std::istreambuf_iterator<char>{page}, std::istreambuf_iterator<char>{}};
        auto tokens = run_tokenizer(page_str);

        expect_eq(tokens,
                std::vector<Token>{DoctypeToken{.name = "html"s},
                        CharacterToken{'\n'},
                        StartTagToken{.tag_name = "html"s},
                        CharacterToken{'\n'},
                        EndTagToken{.tag_name = "html"s},
                        CharacterToken{'\n'},
                        EndOfFileToken{}});
    });

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

    etest::test("script, escaped one dash", [] {
        auto tokens = run_tokenizer("<script><!-- -<</script>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!-- -<"sv);
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

    etest::test("script, double escaped dash", [] {
        auto tokens = run_tokenizer("<script><!--<script>---</script>--></script>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!--<script>---</script>-->"sv);
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

        expect_eq(tokens, std::vector<Token>{CommentToken{.data = " Hello "}, EndOfFileToken{}});
    });

    etest::test("comment, empty", [] {
        auto tokens = run_tokenizer("<!---->");

        expect_eq(tokens, std::vector<Token>{CommentToken{.data = ""}, EndOfFileToken{}});
    });

    etest::test("comment, with dashes and bang", [] {
        auto tokens = run_tokenizer("<!--!-->");

        expect_eq(tokens, std::vector<Token>{CommentToken{.data = "!"}, EndOfFileToken{}});
    });

    etest::test("comment, with new lines", [] {
        auto tokens = run_tokenizer("<!--\nOne\nTwo\n-->");

        expect_eq(tokens, std::vector<Token>{CommentToken{.data = "\nOne\nTwo\n"}, EndOfFileToken{}});
    });

    etest::test("comment, multiple with new lines", [] {
        auto tokens = run_tokenizer("<!--a-->\n<!--b-->\n<!--c-->");

        expect_eq(tokens,
                std::vector<Token>{CommentToken{.data = "a"},
                        CharacterToken{'\n'},
                        CommentToken{.data = "b"},
                        CharacterToken{'\n'},
                        CommentToken{.data = "c"},
                        EndOfFileToken{}});
    });

    etest::test("comment, allowed to end with <!", [] {
        auto tokens = run_tokenizer("<!--My favorite operators are > and <!-->");

        expect_eq(tokens,
                std::vector<Token>{CommentToken{.data = "My favorite operators are > and <!"}, EndOfFileToken{}});
    });

    etest::test("comment, nested comment", [] {
        auto tokens = run_tokenizer("<!--<!---->");

        expect_eq(tokens, std::vector<Token>{CommentToken{.data = "<!--"}, EndOfFileToken{}});
    });

    etest::test("comment, nested comment closed", [] {
        auto tokens = run_tokenizer("<!-- <!-- nested --> -->");

        expect_eq(tokens,
                std::vector<Token>{CommentToken{.data = " <!-- nested "},
                        CharacterToken{' '},
                        CharacterToken{'-'},
                        CharacterToken{'-'},
                        CharacterToken{'>'},
                        EndOfFileToken{}});
    });

    etest::test("comment, abrupt closing in comment start", [] {
        auto tokens = run_tokenizer("<!-->");

        expect_eq(tokens, std::vector<Token>{CommentToken{.data = ""}, EndOfFileToken{}});
    });

    etest::test("comment, abrupt closing in comment start dash", [] {
        auto tokens = run_tokenizer("<!--->");

        expect_eq(tokens, std::vector<Token>{CommentToken{.data = ""}, EndOfFileToken{}});
    });

    etest::test("comment, incorrectly closed comment", [] {
        auto tokens = run_tokenizer("<!--abc--!>");

        expect_eq(tokens, std::vector<Token>{CommentToken{.data = "abc"}, EndOfFileToken{}});
    });

    etest::test("comment, end before comment", [] {
        auto tokens = run_tokenizer("<!--");

        expect_eq(tokens, std::vector<Token>{CommentToken{.data = ""}, EndOfFileToken{}});
    });

    etest::test("comment, eof before comment is closed", [] {
        auto tokens = run_tokenizer("<!--abc");

        expect_eq(tokens, std::vector<Token>{CommentToken{.data = "abc"}, EndOfFileToken{}});
    });

    etest::test("character entity reference, simple", [] {
        auto tokens = run_tokenizer("&lt;");
        expect_eq(tokens, std::vector<Token>{CharacterToken{'<'}, EndOfFileToken{}});
    });

    etest::test("character entity reference, only &", [] {
        auto tokens = run_tokenizer("&");
        expect_eq(tokens, std::vector<Token>{CharacterToken{'&'}, EndOfFileToken{}});
    });

    etest::test("character entity reference, not ascii alphanumeric", [] {
        auto tokens = run_tokenizer("&@");
        expect_eq(tokens, std::vector<Token>{CharacterToken{'&'}, CharacterToken{'@'}, EndOfFileToken{}});
    });

    etest::test("character entity reference, reference to non-ascii glyph", [] {
        auto tokens = run_tokenizer("&div;");
        expect_eq(tokens, std::vector<Token>{CharacterToken{'\xc3'}, CharacterToken{'\xb7'}, EndOfFileToken{}});
        std::string glyph{};
        glyph += std::get<CharacterToken>(tokens[0]).data;
        glyph += std::get<CharacterToken>(tokens[1]).data;
        expect_eq(glyph, "÷"sv);
    });

    etest::test("character entity reference, two unicode code points required", [] {
        auto tokens = run_tokenizer("&acE;");
        expect_eq(tokens,
                std::vector<Token>{CharacterToken{'\xe2'},
                        CharacterToken{'\x88'},
                        CharacterToken{'\xbe'},
                        CharacterToken{'\xcc'},
                        CharacterToken{'\xb3'},
                        EndOfFileToken{}});
        std::string glyph{};
        glyph += std::get<CharacterToken>(tokens[0]).data;
        glyph += std::get<CharacterToken>(tokens[1]).data;
        glyph += std::get<CharacterToken>(tokens[2]).data;
        glyph += std::get<CharacterToken>(tokens[3]).data;
        glyph += std::get<CharacterToken>(tokens[4]).data;
        expect_eq(glyph, "∾̳"sv);
    });

    etest::test("ambiguous ampersand", [] {
        auto tokens = run_tokenizer("&blah;");
        expect_token(tokens, CharacterToken{'&'});
        expect_token(tokens, CharacterToken{'b'});
        expect_token(tokens, CharacterToken{'l'});
        expect_token(tokens, CharacterToken{'a'});
        expect_token(tokens, CharacterToken{'h'});
        expect_token(tokens, CharacterToken{';'});
        expect_token(tokens, EndOfFileToken{});
        expect(tokens.empty());
    });

    etest::test("ambiguous ampersand in attribute", [] {
        auto tokens = run_tokenizer("<p attr='&blah;'>");
        expect_token(tokens, StartTagToken{.tag_name = "p", .attributes = {{"attr", "&blah;"}}});
        expect_token(tokens, EndOfFileToken{});
        expect(tokens.empty());
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
        expect_token(tokens, StartTagToken{.tag_name = "tag", .attributes = {{"a", "b=c"}}});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("attribute, unquoted, eof-in-tag", [] {
        auto tokens = run_tokenizer("<tag a=b");
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("attribute, unquoted, with character reference", [] {
        auto tokens = run_tokenizer("<tag a=&amp>");
        expect_token(tokens, StartTagToken{.tag_name = "tag", .attributes = {{"a", "&"}}});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("attribute, unquoted, unexpected-null-character", [] {
        auto tokens = run_tokenizer("<tag a=\0>"sv);
        expect_token(tokens, StartTagToken{.tag_name = "tag", .attributes = {{"a", kReplacementCharacter}}});
        expect_token(tokens, EndOfFileToken{});
    });

    etest::test("numeric character reference", [] {
        auto tokens = run_tokenizer("&#9731;"); // U+2603: SNOWMAN
        expect_token(tokens, CharacterToken{'\xe2'});
        expect_token(tokens, CharacterToken{'\x98'});
        expect_token(tokens, CharacterToken{'\x83'});
        expect_token(tokens, EndOfFileToken{});
        expect(tokens.empty());
    });

    etest::test("tag closed after attribute name", [] {
        auto tokens = run_tokenizer("<one a><two b>");
        expect_token(tokens, StartTagToken{.tag_name = "one", .attributes = {{"a", ""}}});
        expect_token(tokens, StartTagToken{.tag_name = "two", .attributes = {{"b", ""}}});
        expect_token(tokens, EndOfFileToken{});
        expect(tokens.empty());
    });

    return etest::run_all_tests();
}
