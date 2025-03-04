// SPDX-FileCopyrightText: 2021-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "html2/tokenizer.h"

#include "html2/token.h"

#include "etest/etest2.h"

#include <array>
#include <format>
#include <iterator>
#include <optional>
#include <source_location>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

using namespace std::literals;

using namespace html2;

namespace {

constexpr char const *kReplacementCharacter = "\xef\xbf\xbd";

struct ParseErrorWithLocation {
    ParseError error{};
    SourceLocation location{};
    [[nodiscard]] bool operator==(ParseErrorWithLocation const &) const = default;
};

class TokenizerOutput {
public:
    ~TokenizerOutput() {
        a.expect(tokens.empty(), "Not all tokens were handled", loc);
        a.expect(errors.empty(), "Not all errors were handled", loc);
    }

    etest::IActions &a;
    std::vector<Token> tokens;
    std::vector<ParseErrorWithLocation> errors;
    std::source_location loc;
};

struct Options {
    bool in_html_namespace{true};
    std::optional<html2::State> state_override{};
};

TokenizerOutput run_tokenizer(etest::IActions &a,
        std::string_view input,
        Options const &opts = Options{},
        std::source_location loc = std::source_location::current()) {
    std::vector<Token> tokens;
    std::vector<ParseErrorWithLocation> errors;
    Tokenizer tokenizer{input,
            [&](Tokenizer &the, Token &&t) {
                if (auto const *start_tag = std::get_if<StartTagToken>(&t)) {
                    if (start_tag->tag_name == "script") {
                        the.set_state(State::ScriptData);
                    } else if (start_tag->tag_name == "style") {
                        the.set_state(State::Rawtext);
                    } else if (start_tag->tag_name == "title") {
                        the.set_state(State::Rcdata);
                    }
                }
                tokens.push_back(std::move(t));
            },
            [&](Tokenizer &the, ParseError e) {
                errors.push_back({e, the.current_source_location()});
            }};
    if (opts.state_override) {
        tokenizer.set_state(*opts.state_override);
    }
    tokenizer.set_adjusted_current_node_in_html_namespace(opts.in_html_namespace);
    tokenizer.run();

    return {a, std::move(tokens), std::move(errors), std::move(loc)};
}

void expect_token(
        TokenizerOutput &output, Token const &t, std::source_location const &loc = std::source_location::current()) {
    auto &a = output.a;
    a.require(!output.tokens.empty(), "Unexpected end of token list", loc);
    a.expect_eq(output.tokens.front(), t, {}, loc);
    output.tokens.erase(begin(output.tokens));
}

void expect_text(TokenizerOutput &output,
        std::string_view text,
        std::source_location const &loc = std::source_location::current()) {
    for (auto c : text) {
        expect_token(output, CharacterToken{c}, loc);
    }
}

void expect_error(
        TokenizerOutput &output, ParseError e, std::source_location const &loc = std::source_location::current()) {
    auto &a = output.a;
    a.require(!output.errors.empty(), "Unexpected end of error list", loc);
    a.expect_eq(output.errors.front().error, e, {}, loc);
    output.errors.erase(begin(output.errors));
}

void expect_error(TokenizerOutput &output,
        ParseErrorWithLocation const &e,
        std::source_location const &loc = std::source_location::current()) {
    auto &a = output.a;
    a.require(!output.errors.empty(), "Unexpected end of error list", loc);
    a.expect_eq(output.errors.front(), e, {}, loc);
    output.errors.erase(begin(output.errors));
}

void data_tests(etest::Suite &s) {
    s.add_test("data, unexpected null", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<p>nullp\0"sv);
        expect_token(tokens, StartTagToken{.tag_name = "p"});
        expect_text(tokens, "nullp\0"sv);
        expect_error(tokens, ParseError::UnexpectedNullCharacter);
        expect_token(tokens, EndOfFileToken{});
    });
}

void cdata_tests(etest::Suite &s) {
    s.add_test("cdata, currently in html", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<![CDATA["sv);
        expect_error(tokens, ParseError::CdataInHtmlContent);
        expect_token(tokens, CommentToken{.data = "[CDATA["});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("cdata, eof", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<![CDATA["sv, Options{.in_html_namespace = false});
        expect_error(tokens, html2::ParseError::EofInCdata);
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("cdata, bracket", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<![CDATA[]hello"sv, Options{.in_html_namespace = false});
        expect_error(tokens, html2::ParseError::EofInCdata);
        expect_text(tokens, "]hello");
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("cdata, end", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<![CDATA[]]>"sv, Options{.in_html_namespace = false});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("cdata, end, extra bracket", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<![CDATA[]]]>"sv, Options{.in_html_namespace = false});
        expect_token(tokens, CharacterToken{']'});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("cdata, end, extra text", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<![CDATA[]]a]]>"sv, Options{.in_html_namespace = false});
        expect_text(tokens, "]]a");
        expect_token(tokens, EndOfFileToken{});
    });
}

void doctype_system_keyword_tests(etest::Suite &s) {
    s.add_test("doctype system keyword, single-quoted system identifier, missing space", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<!DOCTYPE HTML SYSTEM'great'>");
        expect_error(tokens, ParseError::MissingWhitespaceAfterDoctypeSystemKeyword);
        expect_token(tokens, DoctypeToken{.name = "html", .system_identifier = "great"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("doctype system keyword, double-quoted system identifier, missing space", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, R"(<!DOCTYPE HTML SYSTEM"great">)");
        expect_error(tokens, ParseError::MissingWhitespaceAfterDoctypeSystemKeyword);
        expect_token(tokens, DoctypeToken{.name = "html", .system_identifier = "great"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("doctype system keyword, missing identifier", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, R"(<!DOCTYPE HTML SYSTEM>)");
        expect_error(tokens, ParseError::MissingDoctypeSystemIdentifier);
        expect_token(tokens, DoctypeToken{.name = "html", .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("doctype system keyword, missing quote before identifier", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, R"(<!DOCTYPE HTML SYSTEMgreat>)");
        expect_error(tokens, ParseError::MissingQuoteBeforeDoctypeSystemIdentifier);
        expect_token(tokens, DoctypeToken{.name = "html", .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("doctype system keyword, eof in doctype", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, R"(<!DOCTYPE HTML SYSTEM)");
        expect_error(tokens, ParseError::EofInDoctype);
        expect_token(tokens, DoctypeToken{.name = "html", .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("doctype before system identifier, single-quoted system identifier", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<!DOCTYPE HTML SYSTEM 'great'>");
        expect_token(tokens, DoctypeToken{.name = "html", .system_identifier = "great"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("doctype before system identifier, double-quoted system identifier", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, R"(<!DOCTYPE HTML SYSTEM "great">)");
        expect_token(tokens, DoctypeToken{.name = "html", .system_identifier = "great"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("doctype before system identifier, more eof in doctype", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, R"(<!DOCTYPE HTML SYSTEM   )");
        expect_error(tokens, ParseError::EofInDoctype);
        expect_token(tokens, DoctypeToken{.name = "html", .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("doctype before system identifier, missing identifier", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, R"(<!DOCTYPE HTML SYSTEM >)");
        expect_error(tokens, ParseError::MissingDoctypeSystemIdentifier);
        expect_token(tokens, DoctypeToken{.name = "html", .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("doctype before system identifier, missing quote before identifier", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, R"(<!DOCTYPE HTML SYSTEM great>)");
        expect_error(tokens, ParseError::MissingQuoteBeforeDoctypeSystemIdentifier);
        expect_token(tokens, DoctypeToken{.name = "html", .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });
}

// These tests set the initial state as normally that would be done from the
// tree-builder wrapping the tokenizer, e.g. when encountering a <style> tag.
void rawtext_tests(etest::Suite &s) {
    s.add_test("rawtext", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<these><aren't><tags!>", Options{.state_override = State::Rawtext});
        expect_text(tokens, "<these><aren't><tags!>");
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("rawtext, unexpected null", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "\0"sv, Options{.state_override = State::Rawtext});
        expect_error(tokens, ParseError::UnexpectedNullCharacter);
        expect_text(tokens, kReplacementCharacter);
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("rawtext inappropriate end tag", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<hello></div>", Options{.state_override = State::Rawtext});
        expect_text(tokens, "<hello></div>");
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("rawtext in style, with attribute", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<style>sometext</style>");
        expect_token(tokens, StartTagToken{.tag_name = "style"});
        expect_text(tokens, "sometext");
        expect_token(tokens, EndTagToken{.tag_name = "style"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("rawtext in style, with attribute", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<style><div></style hello='1'>");
        expect_token(tokens, StartTagToken{.tag_name = "style"});
        expect_text(tokens, "<div>");
        expect_token(tokens, EndTagToken{.tag_name = "style"});
        expect_token(tokens, EndOfFileToken{});
        expect_error(tokens, ParseError::EndTagWithAttributes);
    });

    s.add_test("rawtext in style, self-closing end tag", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<style><div></style/>");
        expect_token(tokens, StartTagToken{.tag_name = "style"});
        expect_text(tokens, "<div>");
        expect_token(tokens, EndTagToken{.tag_name = "style"});
        expect_token(tokens, EndOfFileToken{});
        expect_error(tokens, ParseError::EndTagWithTrailingSolidus);
    });

    s.add_test("rawtext, end tag open, eof", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<hello></", Options{.state_override = State::Rawtext});
        expect_text(tokens, "<hello></");
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("rawtext, end tag name, eof", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<hello></a </b/ </c! </g", Options{.state_override = State::Rawtext});
        expect_text(tokens, "<hello></a </b/ </c! </g");
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("rawtext in style, character reference", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<style>&lt;div&gt;</style>");
        expect_token(tokens, StartTagToken{.tag_name = "style"});
        expect_text(tokens, "&lt;div&gt;");
        expect_token(tokens, EndTagToken{.tag_name = "style"});
        expect_token(tokens, EndOfFileToken{});
    });
}

void rcdata_tests(etest::Suite &s) {
    s.add_test("rcdata", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<these><aren't><tags!>", Options{.state_override = State::Rcdata});
        expect_text(tokens, "<these><aren't><tags!>");
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("rcdata, unexpected null", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "\0"sv, Options{.state_override = State::Rcdata});
        expect_error(tokens, ParseError::UnexpectedNullCharacter);
        expect_text(tokens, kReplacementCharacter);
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("rcdata inappropriate end tag", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<hello></div>", Options{.state_override = State::Rcdata});
        expect_text(tokens, "<hello></div>");
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("rcdata in title, with attribute", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<title>sometext</title>");
        expect_token(tokens, StartTagToken{.tag_name = "title"});
        expect_text(tokens, "sometext");
        expect_token(tokens, EndTagToken{.tag_name = "title"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("rcdata in title, with attribute", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<title><div></title hello='1'>");
        expect_token(tokens, StartTagToken{.tag_name = "title"});
        expect_text(tokens, "<div>");
        expect_token(tokens, EndTagToken{.tag_name = "title"});
        expect_token(tokens, EndOfFileToken{});
        expect_error(tokens, ParseError::EndTagWithAttributes);
    });

    s.add_test("rcdata in title, self-closing end tag", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<title><div></title/>");
        expect_token(tokens, StartTagToken{.tag_name = "title"});
        expect_text(tokens, "<div>");
        expect_token(tokens, EndTagToken{.tag_name = "title"});
        expect_token(tokens, EndOfFileToken{});
        expect_error(tokens, ParseError::EndTagWithTrailingSolidus);
    });

    s.add_test("rcdata, end tag open, eof", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<hello></", Options{.state_override = State::Rcdata});
        expect_text(tokens, "<hello></");
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("rcdata, end tag name, eof", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<hello></a </b/ </c! </g", Options{.state_override = State::Rcdata});
        expect_text(tokens, "<hello></a </b/ </c! </g");
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("rcdata in title, character reference", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<title>&lt;div&gt;</title>");
        expect_token(tokens, StartTagToken{.tag_name = "title"});
        expect_text(tokens, "<div>");
        expect_token(tokens, EndTagToken{.tag_name = "title"});
        expect_token(tokens, EndOfFileToken{});
    });
}

// https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-inbody
// Once a start tag with the tag name "plaintext" has been seen, that will be
// the last token ever seen other than character tokens (and the end-of-file
// token), because there is no way to switch out of the PLAINTEXT state.
void plaintext_tests(etest::Suite &s) {
    s.add_test("plaintext", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "</plaintext>", Options{.state_override = State::Plaintext});
        expect_text(tokens, "</plaintext>");
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("plaintext, null character", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "\0"sv, Options{.state_override = State::Plaintext});
        expect_error(tokens, ParseError::UnexpectedNullCharacter);
        expect_text(tokens, kReplacementCharacter);
        expect_token(tokens, EndOfFileToken{});
    });
}

void source_location_tests(etest::Suite &s) {
    s.add_test("src loc: doctype eof", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<!DOCTYPE HtMl");
        expect_token(tokens, DoctypeToken{.name = "html", .force_quirks = true});
        expect_error(tokens, {ParseError::EofInDoctype, {1, 15}});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("src loc: doctype missing whitespace after public + eof", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<!DOCTYPE a PUBLIC'\n\n\n\n");
        expect_token(tokens, DoctypeToken{.name = "a", .public_identifier = "\n\n\n\n", .force_quirks = true});
        expect_error(tokens, {ParseError::MissingWhitespaceAfterDoctypePublicKeyword, {1, 19}});
        expect_error(tokens, {ParseError::EofInDoctype, {5, 1}});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("src loc: cdata eof", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "\n", {.state_override = State::CdataSection});
        expect_token(tokens, CharacterToken{'\n'});
        expect_error(tokens, {ParseError::EofInCdata, {2, 1}});
        expect_token(tokens, EndOfFileToken{});
    });
}

void tag_open_tests(etest::Suite &s) {
    s.add_test("tag open: eof", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<");
        expect_error(tokens, ParseError::EofBeforeTagName);
        expect_token(tokens, CharacterToken{'<'});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("tag open: question mark is a bogus comment", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<?hello");
        expect_error(tokens, ParseError::UnexpectedQuestionMarkInsteadOfTagName);
        expect_token(tokens, CommentToken{"?hello"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("tag open: invalid first character", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<#bogus");
        expect_error(tokens, ParseError::InvalidFirstCharacterOfTagName);
        expect_text(tokens, "<#bogus");
        expect_token(tokens, EndOfFileToken{});
    });
}

void end_tag_open_tests(etest::Suite &s) {
    s.add_test("end tag open: eof", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "</");
        expect_error(tokens, ParseError::EofBeforeTagName);
        expect_text(tokens, "</");
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("end tag open: missing tag name", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "</>");
        expect_error(tokens, ParseError::MissingEndTagName);
        expect_token(tokens, EndOfFileToken{});
    });
}

void tag_name_tests(etest::Suite &s) {
    s.add_test("tag name: eof", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<imtrappedinabrowserfactorypleasesendhel");
        expect_error(tokens, ParseError::EofInTag);
        expect_token(tokens, EndOfFileToken{});
    });
}

void script_data_escaped_tests(etest::Suite &s) {
    s.add_test("script data escaped: eof", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<script><!-- foo");
        expect_error(tokens, ParseError::EofInScriptHtmlCommentLikeText);
        expect_token(tokens, StartTagToken{"script"});
        expect_text(tokens, "<!-- foo"sv);
        expect_token(tokens, EndOfFileToken{});
    });
}

void script_data_escaped_dash_tests(etest::Suite &s) {
    s.add_test("script data escaped: eof", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<script><!-- foo-");
        expect_error(tokens, ParseError::EofInScriptHtmlCommentLikeText);
        expect_token(tokens, StartTagToken{"script"});
        expect_text(tokens, "<!-- foo-"sv);
        expect_token(tokens, EndOfFileToken{});
    });
}

void script_data_escaped_dash_dash_tests(etest::Suite &s) {
    s.add_test("script data escaped: eof", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<script><!-- foo--");
        expect_error(tokens, ParseError::EofInScriptHtmlCommentLikeText);
        expect_token(tokens, StartTagToken{"script"});
        expect_text(tokens, "<!-- foo--"sv);
        expect_token(tokens, EndOfFileToken{});
    });
}

void script_data_double_escaped_tests(etest::Suite &s) {
    s.add_test("script data double escaped: eof", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<script><!--<script>"sv);
        expect_error(tokens, ParseError::EofInScriptHtmlCommentLikeText);
        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!--<script>");
        expect_token(tokens, EndOfFileToken{});
    });
}

void script_data_double_escaped_dash_tests(etest::Suite &s) {
    s.add_test("script data double escaped dash: eof", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<script><!--<script>-"sv);
        expect_error(tokens, ParseError::EofInScriptHtmlCommentLikeText);
        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!--<script>-");
        expect_token(tokens, EndOfFileToken{});
    });
}

void script_data_double_escaped_dash_dash_tests(etest::Suite &s) {
    s.add_test("script data double escaped dash dash: eof", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<script><!--<script>--"sv);
        expect_error(tokens, ParseError::EofInScriptHtmlCommentLikeText);
        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!--<script>--");
        expect_token(tokens, EndOfFileToken{});
    });
}

void before_attribute_name_tests(etest::Suite &s) {
    s.add_test("before attribute name: =", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<p =hello=13>");
        expect_error(tokens, ParseError::UnexpectedEqualsSignBeforeAttributeName);
        expect_token(tokens, StartTagToken{.tag_name = "p", .attributes{{"=hello", "13"}}});
        expect_token(tokens, EndOfFileToken{});
    });
}

void attribute_name_tests(etest::Suite &s) {
    s.add_test("attribute name: unexpected character", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<p a<b=true>");
        expect_error(tokens, ParseError::UnexpectedCharacterInAttributeName);
        expect_token(tokens, StartTagToken{.tag_name = "p", .attributes{{"a<b", "true"}}});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("attribute name: duplicate attribute", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<p a=1 a=2>");
        expect_error(tokens, ParseError::DuplicateAttribute);
        expect_token(tokens, StartTagToken{.tag_name = "p", .attributes{{"a", "1"}}});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("attribute name: many duplicate attributes", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<p a=1 a=2 a=3>");
        expect_error(tokens, ParseError::DuplicateAttribute);
        expect_error(tokens, ParseError::DuplicateAttribute);
        expect_token(tokens, StartTagToken{.tag_name = "p", .attributes{{"a", "1"}}});
        expect_token(tokens, EndOfFileToken{});
    });
}

void after_attribute_name_tests(etest::Suite &s) {
    s.add_test("after attribute name: eof", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<p a ");
        expect_error(tokens, ParseError::EofInTag);
        expect_token(tokens, EndOfFileToken{});
    });
}

void before_attribute_value_tests(etest::Suite &s) {
    s.add_test("before attribute name: missing value", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<p a=>");
        expect_error(tokens, ParseError::MissingAttributeValue);
        expect_token(tokens, StartTagToken{.tag_name = "p", .attributes{{"a", ""}}});
        expect_token(tokens, EndOfFileToken{});
    });
}

void attribute_value_double_quoted_tests(etest::Suite &s) {
    s.add_test("attribute value double quoted: eof", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, R"(<p a=">)");
        expect_error(tokens, ParseError::EofInTag);
        expect_token(tokens, EndOfFileToken{});
    });
}

void attribute_value_single_quoted_tests(etest::Suite &s) {
    s.add_test("attribute value single quoted: eof", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<p a='>");
        expect_error(tokens, ParseError::EofInTag);
        expect_token(tokens, EndOfFileToken{});
    });
}

void after_attribute_value_quoted_tests(etest::Suite &s) {
    s.add_test("after attribute value quoted: eof", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<p foo='1'");
        expect_error(tokens, ParseError::EofInTag);
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("after attribute value quoted: missing whitespace", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<p foo='1'bar='2'>");
        expect_error(tokens, ParseError::MissingWhitespaceBetweenAttributes);
        expect_token(tokens, StartTagToken{.tag_name = "p", .attributes{{"foo", "1"}, {"bar", "2"}}});
        expect_token(tokens, EndOfFileToken{});
    });
}

void self_closing_start_tag_tests(etest::Suite &s) {
    s.add_test("self-closing start tag: eof", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<p/");
        expect_error(tokens, ParseError::EofInTag);
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("self-closing start tag: unexpected solidus", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<p/ >");
        expect_error(tokens, ParseError::UnexpectedSolidusInTag);
        expect_token(tokens, StartTagToken{"p"});
        expect_token(tokens, EndOfFileToken{});
    });
}

void comment_start_dash_tests(etest::Suite &s) {
    s.add_test("comment start dash: eof", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<!---");
        expect_error(tokens, ParseError::EofInComment);
        expect_token(tokens, CommentToken{});
        expect_token(tokens, EndOfFileToken{});
    });
}

void comment_end_dash_tests(etest::Suite &s) {
    s.add_test("comment end dash: eof", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<!-- -");
        expect_error(tokens, ParseError::EofInComment);
        expect_token(tokens, CommentToken{" "});
        expect_token(tokens, EndOfFileToken{});
    });
}

void comment_end_tests(etest::Suite &s) {
    s.add_test("comment end: eof", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<!-- --");
        expect_error(tokens, ParseError::EofInComment);
        expect_token(tokens, CommentToken{" "});
        expect_token(tokens, EndOfFileToken{});
    });
}

void comment_end_bang_tests(etest::Suite &s) {
    s.add_test("comment end bang: eof", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<!-- --!");
        expect_error(tokens, ParseError::EofInComment);
        expect_token(tokens, CommentToken{" "});
        expect_token(tokens, EndOfFileToken{});
    });
}

} // namespace

int main() {
    etest::Suite s;
    data_tests(s);
    cdata_tests(s);
    doctype_system_keyword_tests(s);
    rawtext_tests(s);
    rcdata_tests(s);
    plaintext_tests(s);
    source_location_tests(s);
    tag_open_tests(s);
    end_tag_open_tests(s);
    tag_name_tests(s);
    script_data_escaped_tests(s);
    script_data_escaped_dash_tests(s);
    script_data_escaped_dash_dash_tests(s);
    script_data_double_escaped_tests(s);
    script_data_double_escaped_dash_tests(s);
    script_data_double_escaped_dash_dash_tests(s);
    before_attribute_name_tests(s);
    attribute_name_tests(s);
    after_attribute_name_tests(s);
    before_attribute_value_tests(s);
    attribute_value_double_quoted_tests(s);
    attribute_value_single_quoted_tests(s);
    after_attribute_value_quoted_tests(s);
    self_closing_start_tag_tests(s);
    comment_start_dash_tests(s);
    comment_end_dash_tests(s);
    comment_end_tests(s);
    comment_end_bang_tests(s);

    s.add_test("script, empty", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<script></script>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("script, upper case tag", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<SCRIPT></SCRIPT>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("script, with code", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<script>code</script>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "code"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("script, unexpected null", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<script>\0</script>"sv);

        expect_error(tokens, ParseError::UnexpectedNullCharacter);
        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, kReplacementCharacter);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("script, with source file attribute", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, R"(<script src="/foo.js"></script>)");

        expect_token(tokens, StartTagToken{.tag_name = "script", .attributes = {{"src", "/foo.js"}}});
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("script, end tag as text", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<script></</script>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "</"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("script, misspelled end tag", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<script></scropt>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "</scropt>"sv);
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("script, almost escaped", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<script><!</script>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("script, almost escaped dash", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<script><!-<</script>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!-<"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("script, escaped", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<script><!-- </script> --></script>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!-- "sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_text(tokens, " -->"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("script, escaped null", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<script><!-- \0 --></script>"sv);

        expect_error(tokens, ParseError::UnexpectedNullCharacter);
        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!-- "s + kReplacementCharacter + " -->");
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("script, escaped one dash", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<script><!-- -<</script>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!-- -<"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("script, escaped dash null", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<script><!-- -\0</script>"sv);

        expect_error(tokens, ParseError::UnexpectedNullCharacter);
        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!-- -"s + kReplacementCharacter);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("script, escaped dash dash null", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<script><!-- --\0</script>"sv);

        expect_error(tokens, ParseError::UnexpectedNullCharacter);
        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!-- --"s + kReplacementCharacter);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("script, escaped one dash and back to escaped", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<script><!-- -x</script>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!-- -x"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("script, escaped upper case", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<script><!--- </SCRIPT> ---></script>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!--- "sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_text(tokens, " --->"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("script, escaped dummy tags", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<script><!-- <</xyz>> --></script>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!-- <</xyz>> -->"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("script, double escaped", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<script><!--<script>code</script>--></script>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!--<script>code</script>-->"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("script, double escaped null", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<script><!--<script>\0</script>--></script>"sv);

        expect_error(tokens, ParseError::UnexpectedNullCharacter);
        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!--<script>"s + kReplacementCharacter + "</script>-->");
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("script, double escaped dash", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<script><!--<script>---</script>--></script>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!--<script>---</script>-->"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("script, double escaped dash null", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<script><!--<script>-\0</script>--></script>"sv);

        expect_error(tokens, ParseError::UnexpectedNullCharacter);
        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!--<script>-"s + kReplacementCharacter + "</script>-->");
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("script, double escaped dash dash null", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<script><!--<script>--\0</script>--></script>"sv);

        expect_error(tokens, ParseError::UnexpectedNullCharacter);
        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!--<script>--"s + kReplacementCharacter + "</script>-->");
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("script, double escaped less than", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<script><!--<script><</xyz>></script>--></script>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!--<script><</xyz>></script>-->"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("script, double escaped dash less than", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<SCRIPT><!--<SCRIPT>-<</SCRIPT>--></SCRIPT>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!--<SCRIPT>-<</SCRIPT>-->"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("script, double escaped dash less than", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<SCRIPT><!--<SCRIPT>-->--></SCRIPT>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!--<SCRIPT>-->-->"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("script, end tag with attribute", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, R"(<script></script src="/foo.js">)");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
        expect_error(tokens, ParseError::EndTagWithAttributes);
    });

    s.add_test("script, misspelled end tag with attribute", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, R"(<script></scropt src="/foo.js">)");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, R"(</scropt src="/foo.js">)"sv);
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("script, self closing end tag", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<script></script/>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
        expect_error(tokens, ParseError::EndTagWithTrailingSolidus);
    });

    s.add_test("script, misspelled self closing end tag", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<script></scropt/>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "</scropt/>"sv);
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("script, escaped end tag open", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<script><!--</>--></script>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!--</>-->"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("script, escaped end tag with attributes", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, R"(<script><!--</script src="/bar.js">--></script>)");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!--"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_text(tokens, "-->"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
        expect_error(tokens, ParseError::EndTagWithAttributes);
    });

    s.add_test("script, misspelled escaped end tag with attributes", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, R"(<script><!--</scropt src="/bar.js">--></script>)");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, R"(<!--</scropt src="/bar.js">-->)"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("script, escaped self closing end tag", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<script><!--</script/>--></script>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!--"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_text(tokens, "-->"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
        expect_error(tokens, ParseError::EndTagWithTrailingSolidus);
    });

    s.add_test("script, misspelled escaped self closing end tag", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<script><!--</scropt/>--></script>");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<!--</scropt/>-->"sv);
        expect_token(tokens, EndTagToken{.tag_name = "script"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("script, eof in less than sign", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<script><");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "<"sv);
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("script, eof in end tag open", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<script></scr");

        expect_token(tokens, StartTagToken{.tag_name = "script"});
        expect_text(tokens, "</scr"sv);
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("comment, simple", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<!-- Hello -->");
        expect_token(tokens, CommentToken{.data = " Hello "});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("comment, bogus open", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<!Hello");
        expect_error(tokens, ParseError::IncorrectlyOpenedComment);
        expect_token(tokens, CommentToken{.data = "Hello"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("comment, empty", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<!---->");
        expect_token(tokens, CommentToken{.data = ""});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("comment, with dashes and bang", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<!--!-->");
        expect_token(tokens, CommentToken{.data = "!"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("comment, with new lines", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<!--\nOne\nTwo\n-->");
        expect_token(tokens, CommentToken{.data = "\nOne\nTwo\n"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("comment, multiple with new lines", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<!--a-->\n<!--b-->\n<!--c-->");
        expect_token(tokens, CommentToken{.data = "a"});
        expect_token(tokens, CharacterToken{'\n'});
        expect_token(tokens, CommentToken{.data = "b"});
        expect_token(tokens, CharacterToken{'\n'});
        expect_token(tokens, CommentToken{.data = "c"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("comment, allowed to end with <!", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<!--My favorite operators are > and <!-->");
        expect_token(tokens, CommentToken{.data = "My favorite operators are > and <!"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("comment, nested comment", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<!--<!---->");
        expect_error(tokens, ParseError::NestedComment);
        expect_token(tokens, CommentToken{.data = "<!--"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("comment, nested comment closed", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<!-- <!-- nested --> -->");
        expect_error(tokens, ParseError::NestedComment);
        expect_token(tokens, CommentToken{.data = " <!-- nested "});
        expect_text(tokens, " -->");
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("comment, abrupt closing in comment start", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<!-->");
        expect_error(tokens, ParseError::AbruptClosingOfEmptyComment);
        expect_token(tokens, CommentToken{.data = ""});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("comment, abrupt closing in comment start dash", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<!--->");
        expect_error(tokens, ParseError::AbruptClosingOfEmptyComment);
        expect_token(tokens, CommentToken{.data = ""});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("comment, incorrectly closed comment", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<!--abc--!>");
        expect_error(tokens, ParseError::IncorrectlyClosedComment);
        expect_token(tokens, CommentToken{.data = "abc"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("comment, end before comment", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<!--");
        expect_error(tokens, ParseError::EofInComment);
        expect_token(tokens, CommentToken{.data = ""});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("comment, eof before comment is closed", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<!--abc");
        expect_error(tokens, ParseError::EofInComment);
        expect_token(tokens, CommentToken{.data = "abc"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("character entity reference, simple", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "&lt;");
        expect_token(tokens, CharacterToken{'<'});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("character entity reference, only &", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "&");
        expect_token(tokens, CharacterToken{'&'});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("character entity reference, not ascii alphanumeric", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "&@");
        expect_text(tokens, "&@");
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("character entity reference, reference to non-ascii glyph", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "&div;");
        expect_text(tokens, "\xc3\xb7"sv);
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("character entity reference, two unicode code points required", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "&acE;");
        expect_text(tokens, "\xe2\x88\xbe\xcc\xb3"sv);
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("ambiguous ampersand", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "&blah;");
        expect_text(tokens, "&blah;");
        expect_error(tokens, ParseError::UnknownNamedCharacterReference);
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("ambiguous ampersand in attribute", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<p attr='&blah;'>");
        expect_token(tokens, StartTagToken{.tag_name = "p", .attributes = {{"attr", "&blah;"}}});
        expect_error(tokens, ParseError::UnknownNamedCharacterReference);
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("attribute, one attribute single quoted", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<tag a='b'>");

        expect_token(tokens, StartTagToken{.tag_name = "tag", .attributes = {{"a", "b"}}});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("attribute, one attribute double quoted", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, R"(<tag a="b">)");

        expect_token(tokens, StartTagToken{.tag_name = "tag", .attributes = {{"a", "b"}}});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("attribute, one uppercase attribute", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, R"(<tag ATTRIB="ABC123">)");

        expect_token(tokens, StartTagToken{.tag_name = "tag", .attributes = {{"attrib", "ABC123"}}});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("attribute, multiple attributes", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, R"(<tag  foo="bar" A='B'  value='321'>)");

        expect_token(
                tokens, StartTagToken{.tag_name = "tag", .attributes = {{"foo", "bar"}, {"a", "B"}, {"value", "321"}}});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("attribute, one attribute unquoted", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<tag a=b>");
        expect_token(tokens, StartTagToken{.tag_name = "tag", .attributes = {{"a", "b"}}});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("attribute, multiple attributes unquoted", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<tag a=b c=d>");
        expect_token(tokens, StartTagToken{.tag_name = "tag", .attributes = {{"a", "b"}, {"c", "d"}}});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("attribute, multiple attributes unquoted", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<tag a=b c=d>");
        expect_token(tokens, StartTagToken{.tag_name = "tag", .attributes = {{"a", "b"}, {"c", "d"}}});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("attribute, unexpected-character-in-unquoted-attribute", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<tag a=b=c>");
        expect_error(tokens, ParseError::UnexpectedCharacterInUnquotedAttributeValue);
        expect_token(tokens, StartTagToken{.tag_name = "tag", .attributes = {{"a", "b=c"}}});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("attribute, unquoted, eof-in-tag", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<tag a=b");
        expect_error(tokens, ParseError::EofInTag);
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("attribute, unquoted, with character reference", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<tag a=&amp>");
        expect_token(tokens, StartTagToken{.tag_name = "tag", .attributes = {{"a", "&"}}});
        expect_error(tokens, ParseError::MissingSemicolonAfterCharacterReference);
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("attribute, unquoted, unexpected-null-character", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<tag a=\0>"sv);
        expect_error(tokens, ParseError::UnexpectedNullCharacter);
        expect_token(tokens, StartTagToken{.tag_name = "tag", .attributes = {{"a", kReplacementCharacter}}});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("numeric character reference", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "&#9731;"); // U+2603: SNOWMAN
        expect_text(tokens, "\xe2\x98\x83");
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("numeric character reference, control with replacement", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "&#x8A;");
        expect_text(tokens, "\xc5\xa0"); // U+0160: LATIN CAPITAL LETTER S WITH CARON
        expect_error(tokens, ParseError::ControlCharacterReference);
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("numeric character reference, no digits", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "&#b;");
        expect_text(tokens, "&#b;");
        expect_error(tokens, ParseError::AbsenceOfDigitsInNumericCharacterReference);
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("numeric character reference, eof", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "&#9731"); // U+2603: SNOWMAN
        expect_text(tokens, "\xe2\x98\x83");
        expect_error(tokens, ParseError::MissingSemicolonAfterCharacterReference);
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("numeric character reference, missing semicolon", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "&#9731b"); // U+2603: SNOWMAN
        expect_text(tokens, "\xe2\x98\x83");
        expect_text(tokens, "b");
        expect_error(tokens, ParseError::MissingSemicolonAfterCharacterReference);
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("numeric character reference, null", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "&#0;");
        expect_text(tokens, kReplacementCharacter);
        expect_error(tokens, ParseError::NullCharacterReference);
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("numeric character reference, outside unicode range", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "&#x11ffff;");
        expect_text(tokens, kReplacementCharacter);
        expect_error(tokens, ParseError::CharacterReferenceOutsideUnicodeRange);
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("numeric character reference, very outside unicode range", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "&#x10000000000000041;");
        expect_text(tokens, kReplacementCharacter);
        expect_error(tokens, ParseError::CharacterReferenceOutsideUnicodeRange);
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("numeric character reference, surrogate", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "&#xd900;");
        expect_text(tokens, kReplacementCharacter);
        expect_error(tokens, ParseError::SurrogateCharacterReference);
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("numeric character reference, noncharacter", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "&#xffff;");
        expect_text(tokens, "\xef\xbf\xbf");
        expect_error(tokens, ParseError::NoncharacterCharacterReference);
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("hexadecimal character reference", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "&#x2721;"); // U+2721
        expect_text(tokens, "\xe2\x9c\xa1");
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("hexadecimal character reference, upper hex digits", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "&#x27FF;"); // U+27FF
        expect_text(tokens, "\xe2\x9f\xbf");
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("hexadecimal character reference, lower hex digits", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "&#x27ff;"); // U+27FF
        expect_text(tokens, "\xe2\x9f\xbf");
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("hexadecimal character reference, no semicolon", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "&#x27ff "); // U+27FF
        expect_error(tokens, ParseError::MissingSemicolonAfterCharacterReference);
        expect_text(tokens, "\xe2\x9f\xbf "); // Note the bonus space.
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("hexadecimal character reference, abrupt end", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "&#x27ff"); // U+27FF
        expect_error(tokens, ParseError::MissingSemicolonAfterCharacterReference);
        expect_text(tokens, "\xe2\x9f\xbf");
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("hexadecimal character reference, no digits", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "&#xG;");
        expect_error(tokens, ParseError::AbsenceOfDigitsInNumericCharacterReference);
        expect_text(tokens, "&#xG;");
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("character reference, c0 control character", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "&#x01;");
        expect_error(tokens, ParseError::ControlCharacterReference);
        expect_text(tokens, "\x01");
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("doctype, eof after name", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<!doctype html ");
        expect_error(tokens, ParseError::EofInDoctype);
        expect_token(tokens, DoctypeToken{.name = "html", .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("doctype, closing tag after whitespace", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<!doctype html  >");
        expect_token(tokens, DoctypeToken{.name = "html"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("doctype, bogus doctype", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<!doctype html bogus>");
        expect_error(tokens, ParseError::InvalidCharacterSequenceAfterDoctypeName);
        expect_token(tokens, DoctypeToken{.name = "html", .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("doctype, bogus doctype, null character and eof", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<!doctype html b\0gus"sv);
        expect_error(tokens, ParseError::InvalidCharacterSequenceAfterDoctypeName);
        expect_error(tokens, ParseError::UnexpectedNullCharacter);
        expect_token(tokens, DoctypeToken{.name = "html", .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    for (char quote : std::array{'\'', '"'}) {
        auto type = quote == '"' ? "double"sv : "single"sv;

        s.add_test(std::format("doctype, {}-quoted public identifier", type), [=](etest::IActions &a) {
            auto tokens = run_tokenizer(a, std::format("<!DOCTYPE HTML PUBLIC {0}great{0}>", quote));
            expect_token(tokens, DoctypeToken{.name = "html", .public_identifier = "great"});
            expect_token(tokens, EndOfFileToken{});
        });

        s.add_test(
                std::format("doctype, {}-quoted public identifier, missing whitespace", type), [=](etest::IActions &a) {
                    auto tokens = run_tokenizer(a, std::format("<!DOCTYPE HTML PUBLIC{0}great{0}>", quote));
                    expect_error(tokens, ParseError::MissingWhitespaceAfterDoctypePublicKeyword);
                    expect_token(tokens, DoctypeToken{.name = "html", .public_identifier = "great"});
                    expect_token(tokens, EndOfFileToken{});
                });

        s.add_test(std::format("doctype, {}-quoted public identifier, eof", type), [=](etest::IActions &a) {
            auto tokens = run_tokenizer(a, std::format("<!DOCTYPE HTML PUBLIC {0}great", quote));
            expect_error(tokens, ParseError::EofInDoctype);
            expect_token(tokens, DoctypeToken{.name = "html", .public_identifier = "great", .force_quirks = true});
            expect_token(tokens, EndOfFileToken{});
        });

        s.add_test(std::format("doctype, {}-quoted public identifier, abrupt end", type), [=](etest::IActions &a) {
            auto tokens = run_tokenizer(a, std::format("<!DOCTYPE HTML PUBLIC {0}great>", quote));
            expect_error(tokens, ParseError::AbruptDoctypePublicIdentifier);
            expect_token(tokens, DoctypeToken{.name = "html", .public_identifier = "great", .force_quirks = true});
            expect_token(tokens, EndOfFileToken{});
        });

        s.add_test(std::format("doctype, {}-quoted public identifier, null", type), [=](etest::IActions &a) {
            auto tokens = run_tokenizer(a, std::format("<!DOCTYPE HTML PUBLIC {0}gre\0t{0}>"sv, quote));
            expect_error(tokens, ParseError::UnexpectedNullCharacter);
            expect_token(
                    tokens, DoctypeToken{.name = "html", .public_identifier = "gre"s + kReplacementCharacter + "t"});
            expect_token(tokens, EndOfFileToken{});
        });

        s.add_test(std::format("doctype, {}-quoted system identifier", type), [=](etest::IActions &a) {
            auto tokens = run_tokenizer(a, std::format("<!DOCTYPE HTML PUBLIC 'great' {0}hello{0}>", quote));
            expect_token(
                    tokens, DoctypeToken{.name = "html", .public_identifier = "great", .system_identifier = "hello"});
            expect_token(tokens, EndOfFileToken{});
        });

        s.add_test(std::format("doctype, {}-quoted system identifier, unexpected null", type), [=](etest::IActions &a) {
            auto tokens = run_tokenizer(a, std::format("<!DOCTYPE HTML PUBLIC 'great' {0}n\0{0}>"sv, quote));
            expect_error(tokens, ParseError::UnexpectedNullCharacter);
            expect_token(tokens,
                    DoctypeToken{.name = "html",
                            .public_identifier = "great",
                            .system_identifier = "n"s + kReplacementCharacter});
            expect_token(tokens, EndOfFileToken{});
        });

        s.add_test(
                std::format("doctype, {}-quoted system identifier, missing whitespace", type), [=](etest::IActions &a) {
                    auto tokens = run_tokenizer(a, std::format("<!DOCTYPE HTML PUBLIC 'great'{0}hello{0}>", quote));
                    expect_error(tokens, ParseError::MissingWhitespaceBetweenDoctypePublicAndSystemIdentifiers);
                    expect_token(tokens,
                            DoctypeToken{.name = "html", .public_identifier = "great", .system_identifier = "hello"});
                    expect_token(tokens, EndOfFileToken{});
                });

        s.add_test(std::format("doctype, {}-quoted system identifier, eof", type), [=](etest::IActions &a) {
            auto tokens = run_tokenizer(a, std::format("<!DOCTYPE HTML PUBLIC 'great' {0}hell", quote));
            expect_error(tokens, ParseError::EofInDoctype);
            expect_token(tokens,
                    DoctypeToken{.name = "html",
                            .public_identifier = "great",
                            .system_identifier = "hell",
                            .force_quirks = true});
            expect_token(tokens, EndOfFileToken{});
        });

        s.add_test(std::format("doctype, {}-quoted system identifier, abrupt end", type), [=](etest::IActions &a) {
            auto tokens = run_tokenizer(a, std::format("<!DOCTYPE HTML PUBLIC 'great' {0}hell>", quote));
            expect_error(tokens, ParseError::AbruptDoctypeSystemIdentifier);
            expect_token(tokens,
                    DoctypeToken{.name = "html",
                            .public_identifier = "great",
                            .system_identifier = "hell",
                            .force_quirks = true});
            expect_token(tokens, EndOfFileToken{});
        });
    }

    s.add_test("doctype, system identifier, missing quote", [=](etest::IActions &a) {
        auto tokens = run_tokenizer(a, R"(<!DOCTYPE HTML PUBLIC "great" hello>)");
        expect_error(tokens, ParseError::MissingQuoteBeforeDoctypeSystemIdentifier);
        expect_token(tokens, DoctypeToken{.name = "html", .public_identifier = "great", .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("doctype, after system identifier, eof", [=](etest::IActions &a) {
        auto tokens = run_tokenizer(a, R"(<!DOCTYPE HTML PUBLIC "great" "hello" )");
        expect_error(tokens, ParseError::EofInDoctype);
        expect_token(tokens,
                DoctypeToken{.name = "html",
                        .public_identifier = "great",
                        .system_identifier = "hello",
                        .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("doctype, after system identifier, unexpected character", [=](etest::IActions &a) {
        auto tokens = run_tokenizer(a, R"(<!DOCTYPE HTML PUBLIC "great" "hello" ohno>)");
        expect_error(tokens, ParseError::UnexpectedCharacterAfterDoctypeSystemIdentifier);
        expect_token(tokens, DoctypeToken{.name = "html", .public_identifier = "great", .system_identifier = "hello"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("doctype, between public and system identifiers, eof", [=](etest::IActions &a) {
        auto tokens = run_tokenizer(a, R"(<!DOCTYPE HTML PUBLIC "great"  )");
        expect_error(tokens, ParseError::EofInDoctype);
        expect_token(tokens, DoctypeToken{.name = "html", .public_identifier = "great", .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("doctype, between public and system identifiers", [=](etest::IActions &a) {
        auto tokens = run_tokenizer(a, R"(<!DOCTYPE HTML PUBLIC "great" >)");
        expect_token(tokens, DoctypeToken{.name = "html", .public_identifier = "great"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("doctype, public identifier, missing quotes", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<!DOCTYPE HTML PUBLIC great>");
        expect_error(tokens, ParseError::MissingQuoteBeforeDoctypePublicIdentifier);
        expect_token(tokens, DoctypeToken{.name = "html", .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("doctype, public identifier, no space", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<!DOCTYPE HTML PUBLICgreat>");
        expect_error(tokens, ParseError::MissingQuoteBeforeDoctypePublicIdentifier);
        expect_token(tokens, DoctypeToken{.name = "html", .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("doctype, public identifier, no space", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, R"(<!DOCTYPE HTML PUBLIC "great"bad>)");
        expect_error(tokens, ParseError::MissingQuoteBeforeDoctypeSystemIdentifier);
        expect_token(tokens, DoctypeToken{.name = "html", .public_identifier = "great", .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("doctype, public keyword, eof", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<!DOCTYPE HTML PUBLIC");
        expect_error(tokens, ParseError::EofInDoctype);
        expect_token(tokens, DoctypeToken{.name = "html", .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("doctype, public keyword, missing identifier", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<!DOCTYPE HTML PUBLIC>");
        expect_error(tokens, ParseError::MissingDoctypePublicIdentifier);
        expect_token(tokens, DoctypeToken{.name = "html", .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("doctype, after public keyword, eof", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<!DOCTYPE HTML PUBLIC  ");
        expect_error(tokens, ParseError::EofInDoctype);
        expect_token(tokens, DoctypeToken{.name = "html", .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("doctype, public keyword but no identifier", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<!DOCTYPE HTML PUBLIC >");
        expect_error(tokens, ParseError::MissingDoctypePublicIdentifier);
        expect_token(tokens, DoctypeToken{.name = "html", .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("doctype, eof after public identifier", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, R"(<!DOCTYPE HTML PUBLIC "great")");
        expect_error(tokens, ParseError::EofInDoctype);
        expect_token(tokens, DoctypeToken{.name = "html", .public_identifier = "great", .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("tag closed after attribute name", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<one a><two b>");
        expect_token(tokens, StartTagToken{.tag_name = "one", .attributes = {{"a", ""}}});
        expect_token(tokens, StartTagToken{.tag_name = "two", .attributes = {{"b", ""}}});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("pages served as xml don't break everything", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<?xml?><!DOCTYPE HTML>");
        expect_error(tokens, ParseError::UnexpectedQuestionMarkInsteadOfTagName);
        expect_token(tokens, CommentToken{"?xml?"});
        expect_token(tokens, DoctypeToken{.name = "html"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("invalid end tag open, eof", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "</!bogus");
        expect_error(tokens, ParseError::InvalidFirstCharacterOfTagName);
        expect_token(tokens, CommentToken{.data = "!bogus"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("invalid end tag open, unexpected null", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "</!bogu\0>"sv);
        expect_error(tokens, ParseError::InvalidFirstCharacterOfTagName);
        expect_error(tokens, ParseError::UnexpectedNullCharacter);
        expect_token(tokens, CommentToken{.data = "!bogu"s + kReplacementCharacter});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("tag name, unexpected null", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<hell\0>"sv);
        expect_error(tokens, ParseError::UnexpectedNullCharacter);
        expect_token(tokens, StartTagToken{.tag_name{"hell"s + kReplacementCharacter}});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("attribute name, unexpected null", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<hello a\0>"sv);
        expect_error(tokens, ParseError::UnexpectedNullCharacter);
        expect_token(tokens, StartTagToken{.tag_name{"hello"s}, .attributes{{"a"s + kReplacementCharacter, ""}}});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("attribute value, unexpected null", [](etest::IActions &a) {
        for (auto html : {"<a b=\"\0\">"sv, "<a b='\0'>"sv}) {
            auto tokens = run_tokenizer(a, html);
            expect_error(tokens, ParseError::UnexpectedNullCharacter);
            expect_token(tokens, StartTagToken{.tag_name{"a"s}, .attributes{{"b"s, kReplacementCharacter}}});
            expect_token(tokens, EndOfFileToken{});
        }
    });

    s.add_test("comment, unexpected null", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<!--\0-->"sv);
        expect_error(tokens, ParseError::UnexpectedNullCharacter);
        expect_token(tokens, CommentToken{.data{kReplacementCharacter}});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("before doctype name, unexpected null", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<!doctype \0hi>"sv);
        expect_error(tokens, ParseError::UnexpectedNullCharacter);
        expect_token(tokens, DoctypeToken{.name{kReplacementCharacter + "hi"s}});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("doctype name, unexpected null", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<!doctype hi\0>"sv);
        expect_error(tokens, ParseError::UnexpectedNullCharacter);
        expect_token(tokens, DoctypeToken{.name{"hi"s + kReplacementCharacter}});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("doctype, eof", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<!doctype"sv);
        expect_error(tokens, ParseError::EofInDoctype);
        expect_token(tokens, DoctypeToken{.force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("doctype, missing doctype name", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<!doctype>"sv);
        expect_error(tokens, ParseError::MissingDoctypeName);
        expect_token(tokens, DoctypeToken{.force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("doctype, missing whitespace before doctype name", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<!doctypelol>"sv);
        expect_error(tokens, ParseError::MissingWhitespaceBeforeDoctypeName);
        expect_token(tokens, DoctypeToken{.name = "lol"});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("before doctype name, eof", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<!doctype "sv);
        expect_error(tokens, ParseError::EofInDoctype);
        expect_token(tokens, DoctypeToken{.force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("doctype name, eof", [](etest::IActions &a) {
        auto tokens = run_tokenizer(a, "<!doctype hi"sv);
        expect_error(tokens, ParseError::EofInDoctype);
        expect_token(tokens, DoctypeToken{.name = "hi", .force_quirks = true});
        expect_token(tokens, EndOfFileToken{});
    });

    s.add_test("to_string(ParseError)", [](etest::IActions &a) {
        // This test will fail if we add new first or last errors, but that's fine.
        constexpr auto kFirstError = ParseError::AbruptClosingOfEmptyComment;
        constexpr auto kLastError = ParseError::UnknownNamedCharacterReference;

        auto error = static_cast<int>(kFirstError);
        a.expect_eq(error, 0);

        while (error <= static_cast<int>(kLastError)) {
            a.expect(to_string(static_cast<ParseError>(error)) != "Unknown error",
                    std::to_string(error) + " is missing an error message");
            error += 1;
        }

        a.expect_eq(to_string(static_cast<ParseError>(error + 1)), "Unknown error");
    });

    return s.run();
}
