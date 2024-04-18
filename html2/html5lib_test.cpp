// SPDX-FileCopyrightText: 2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "html2/token.h"
#include "html2/tokenizer.h"

#include "etest/etest2.h"

#include <simdjson.h> // IWYU pragma: keep

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace {
struct Error {
    html2::ParseError error{};
    html2::SourceLocation location{};

    // TODO(robinlinden): Check line and column as well.
    [[nodiscard]] constexpr bool operator==(Error const &e) const { return error == e.error; }
};

std::pair<std::vector<html2::Token>, std::vector<Error>> tokenize(
        std::string_view input, html2::State state, std::optional<std::string_view> const &last_start_tag) {
    std::vector<html2::Token> tokens;
    std::vector<Error> errors;
    bool last_start_tag_set = true;

    // Patch the input so that we can set the last seen start tag without adding
    // a setter that should only really be used in tests to html2::Tokenizer.
    std::string real_input;
    if (last_start_tag) {
        last_start_tag_set = false;
        std::stringstream ss;
        ss << "<" << *last_start_tag << ">" << input;
        real_input = std::move(ss).str();
    } else {
        real_input = input;
    }

    html2::Tokenizer tokenizer{real_input,
            [&](html2::Tokenizer &t, html2::Token token) {
                // The expected token output doesn't contain eof tokens.
                if (std::holds_alternative<html2::EndOfFileToken>(token)) {
                    return;
                }

                if (!last_start_tag_set) {
                    assert(std::holds_alternative<html2::StartTagToken>(token));
                    last_start_tag_set = true;
                    t.set_state(state);
                    return;
                }

                if (auto const *start_tag = std::get_if<html2::StartTagToken>(&token);
                        start_tag != nullptr && start_tag->tag_name == "script") {
                    t.set_state(html2::State::ScriptData);
                }

                tokens.push_back(std::move(token));
            },
            [&](html2::Tokenizer &t, html2::ParseError error) {
                errors.push_back({error, t.current_source_location()});
            }};

    // If we need to hack the input to set the start tag, the state-override
    // should only take effect after seeing that first start tag.
    if (last_start_tag_set) {
        tokenizer.set_state(state);
    }

    tokenizer.run();

    return {std::move(tokens), std::move(errors)};
}

// NOLINTBEGIN(misc-include-cleaner): What you're meant to include from
// simdjson depends on things like the architecture you're compiling for.
// This is handled automagically with detection macros inside simdjson.
std::vector<html2::Token> to_html2_tokens(simdjson::ondemand::array tokens) {
    constexpr auto kGetOptionalStr = [](simdjson::ondemand::value v) -> std::optional<std::string> {
        if (v.is_null()) {
            return std::nullopt;
        }
        return std::string{v.get_string().value()};
    };

    std::vector<html2::Token> result;
    for (auto token : tokens) {
        auto it = token.begin().value();
        auto kind = (*it).get_string().value();
        if (kind == "DOCTYPE") {
            auto name = kGetOptionalStr((*++it).value());
            auto public_id = kGetOptionalStr((*++it).value());
            auto system_id = kGetOptionalStr((*++it).value());
            // The json has "correctness" instead of "force quirks", so we negate it.
            auto force_quirks = !(*++it).value().get_bool().value();
            result.push_back(html2::DoctypeToken{
                    std::move(name),
                    std::move(public_id),
                    std::move(system_id),
                    force_quirks,
            });
            continue;
        }

        if (kind == "Comment") {
            result.push_back(html2::CommentToken{std::string{(*++it).value().get_string().value()}});
            continue;
        }

        if (kind == "StartTag") {
            html2::StartTagToken start{std::string{(*++it).value().get_string().value()}};
            auto attrs = (*++it).value().get_object().value();
            for (auto attr : attrs) {
                start.attributes.push_back({
                        std::string{attr.unescaped_key().value()},
                        std::string{attr.value().get_string().value()},
                });
            }

            if (++it != simdjson::ondemand::array_iterator{}) {
                start.self_closing = (*it).value().get_bool().value();
            }

            result.push_back(std::move(start));
            continue;
        }

        if (kind == "EndTag") {
            result.push_back(html2::EndTagToken{std::string{(*++it).value().get_string().value()}});
            continue;
        }

        if (kind == "Character") {
            auto characters = (*++it).value().get_string().value();
            for (auto c : characters) {
                result.push_back(html2::CharacterToken{c});
            }
            continue;
        }

        std::cerr << "Unknown token kind: " << kind << '\n';
        std::abort();
    }

    return result;
}

std::optional<html2::State> to_state(std::string_view state_name) {
    if (state_name == "Data state") {
        return html2::State::Data;
    }

    if (state_name == "RCDATA state") {
        return html2::State::Rcdata;
    }

    if (state_name == "RAWTEXT state") {
        return html2::State::Rawtext;
    }

    if (state_name == "Script data state") {
        return html2::State::ScriptData;
    }

    if (state_name == "PLAINTEXT state") {
        return html2::State::Plaintext;
    }

    if (state_name == "CDATA section state") {
        return html2::State::CdataSection;
    }

    return std::nullopt;
}

std::optional<html2::ParseError> to_parse_error(std::string_view error_name) {
    if (error_name == "abrupt-closing-of-empty-comment") {
        return html2::ParseError::AbruptClosingOfEmptyComment;
    }

    if (error_name == "abrupt-doctype-public-identifier") {
        return html2::ParseError::AbruptDoctypePublicIdentifier;
    }

    if (error_name == "abrupt-doctype-system-identifier") {
        return html2::ParseError::AbruptDoctypeSystemIdentifier;
    }

    if (error_name == "absence-of-digits-in-numeric-character-reference") {
        return html2::ParseError::AbsenceOfDigitsInNumericCharacterReference;
    }

    if (error_name == "cdata-in-html-content") {
        return html2::ParseError::CdataInHtmlContent;
    }

    if (error_name == "character-reference-outside-unicode-range") {
        return html2::ParseError::CharacterReferenceOutsideUnicodeRange;
    }

    if (error_name == "control-character-reference") {
        return html2::ParseError::ControlCharacterReference;
    }

    if (error_name == "duplicate-attribute") {
        return html2::ParseError::DuplicateAttribute;
    }

    if (error_name == "end-tag-with-attributes") {
        return html2::ParseError::EndTagWithAttributes;
    }

    if (error_name == "end-tag-with-trailing-solidus") {
        return html2::ParseError::EndTagWithTrailingSolidus;
    }

    if (error_name == "eof-before-tag-name") {
        return html2::ParseError::EofBeforeTagName;
    }

    if (error_name == "eof-in-cdata") {
        return html2::ParseError::EofInCdata;
    }

    if (error_name == "eof-in-comment") {
        return html2::ParseError::EofInComment;
    }

    if (error_name == "eof-in-doctype") {
        return html2::ParseError::EofInDoctype;
    }

    if (error_name == "eof-in-script-html-comment-like-text") {
        return html2::ParseError::EofInScriptHtmlCommentLikeText;
    }

    if (error_name == "eof-in-tag") {
        return html2::ParseError::EofInTag;
    }

    if (error_name == "incorrectly-closed-comment") {
        return html2::ParseError::IncorrectlyClosedComment;
    }

    if (error_name == "incorrectly-opened-comment") {
        return html2::ParseError::IncorrectlyOpenedComment;
    }

    if (error_name == "invalid-character-sequence-after-doctype-name") {
        return html2::ParseError::InvalidCharacterSequenceAfterDoctypeName;
    }

    if (error_name == "invalid-first-character-of-tag-name") {
        return html2::ParseError::InvalidFirstCharacterOfTagName;
    }

    if (error_name == "missing-attribute-value") {
        return html2::ParseError::MissingAttributeValue;
    }

    if (error_name == "missing-doctype-name") {
        return html2::ParseError::MissingDoctypeName;
    }

    if (error_name == "missing-doctype-public-identifier") {
        return html2::ParseError::MissingDoctypePublicIdentifier;
    }

    if (error_name == "missing-doctype-system-identifier") {
        return html2::ParseError::MissingDoctypeSystemIdentifier;
    }

    if (error_name == "missing-end-tag-name") {
        return html2::ParseError::MissingEndTagName;
    }

    if (error_name == "missing-quote-before-doctype-public-identifier") {
        return html2::ParseError::MissingQuoteBeforeDoctypePublicIdentifier;
    }

    if (error_name == "missing-quote-before-doctype-system-identifier") {
        return html2::ParseError::MissingQuoteBeforeDoctypeSystemIdentifier;
    }

    if (error_name == "missing-semicolon-after-character-reference") {
        return html2::ParseError::MissingSemicolonAfterCharacterReference;
    }

    if (error_name == "missing-whitespace-after-doctype-public-keyword") {
        return html2::ParseError::MissingWhitespaceAfterDoctypePublicKeyword;
    }

    if (error_name == "missing-whitespace-after-doctype-system-keyword") {
        return html2::ParseError::MissingWhitespaceAfterDoctypeSystemKeyword;
    }

    if (error_name == "missing-whitespace-before-doctype-name") {
        return html2::ParseError::MissingWhitespaceBeforeDoctypeName;
    }

    if (error_name == "missing-whitespace-between-attributes") {
        return html2::ParseError::MissingWhitespaceBetweenAttributes;
    }

    if (error_name == "missing-whitespace-between-doctype-public-and-system-identifiers") {
        return html2::ParseError::MissingWhitespaceBetweenDoctypePublicAndSystemIdentifiers;
    }

    if (error_name == "nested-comment") {
        return html2::ParseError::NestedComment;
    }

    if (error_name == "noncharacter-character-reference") {
        return html2::ParseError::NoncharacterCharacterReference;
    }

    if (error_name == "null-character-reference") {
        return html2::ParseError::NullCharacterReference;
    }

    if (error_name == "surrogate-character-reference") {
        return html2::ParseError::SurrogateCharacterReference;
    }

    if (error_name == "unexpected-character-after-doctype-system-identifier") {
        return html2::ParseError::UnexpectedCharacterAfterDoctypeSystemIdentifier;
    }

    if (error_name == "unexpected-character-in-attribute-name") {
        return html2::ParseError::UnexpectedCharacterInAttributeName;
    }

    if (error_name == "unexpected-character-in-unquoted-attribute-value") {
        return html2::ParseError::UnexpectedCharacterInUnquotedAttributeValue;
    }

    if (error_name == "unexpected-equals-sign-before-attribute-name") {
        return html2::ParseError::UnexpectedEqualsSignBeforeAttributeName;
    }

    if (error_name == "unexpected-null-character") {
        return html2::ParseError::UnexpectedNullCharacter;
    }

    if (error_name == "unexpected-question-mark-instead-of-tag-name") {
        return html2::ParseError::UnexpectedQuestionMarkInsteadOfTagName;
    }

    if (error_name == "unexpected-solidus-in-tag") {
        return html2::ParseError::UnexpectedSolidusInTag;
    }

    if (error_name == "unknown-named-character-reference") {
        return html2::ParseError::UnknownNamedCharacterReference;
    }

    std::cerr << "Unhandled error: " << error_name << '\n';
    return std::nullopt;
}

std::optional<Error> to_error(simdjson::ondemand::value error) {
    auto code = error["code"].get_string().value();
    if (code == "control-character-in-input-stream" || code == "noncharacter-in-input-stream") {
        // TODO(robinlinden): Handle.
        std::cerr << "Unhandled error: " << code << '\n';
        return std::nullopt;
    }

    auto parse_error = to_parse_error(code).value();
    auto line = error["line"].get_uint64().value();
    auto col = error["col"].get_uint64().value();
    return Error{
            parse_error,
            {static_cast<int>(line), static_cast<int>(col)},
    };
}

std::optional<std::vector<Error>> to_errors(simdjson::ondemand::array errors) {
    std::vector<Error> result;
    for (auto error : errors) {
        auto maybe_error = to_error(error.value());
        if (!maybe_error.has_value()) {
            return std::nullopt;
        }

        result.push_back(*maybe_error);
    }

    return result;
}

} // namespace

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "No test file provided\n";
        return 1;
    }

    auto json = simdjson::padded_string::load(argv[1]);
    if (json.error() != simdjson::SUCCESS) {
        std::cerr << "Error loading test file: " << json.error() << '\n';
        return 1;
    }

    etest::Suite s;

    simdjson::ondemand::parser parser;
    simdjson::ondemand::document doc = parser.iterate(json);
    auto tests = doc.find_field("tests").get_array().value();
    for (auto test : tests) {
        auto name = test["description"].get_string().value();

        // TOOD(robinlinden): Don't skip these.
        if (test["doubleEscaped"].error() == simdjson::SUCCESS) {
            continue;
        }

        std::vector<html2::State> initial_states{html2::State::Data};

        if (test["initialStates"].error() == simdjson::SUCCESS) {
            initial_states.clear();

            auto state_names = test["initialStates"].get_array().value();
            for (auto state_name : state_names) {
                auto state = to_state(state_name.get_string().value());
                if (!state.has_value()) {
                    std::cerr << "Unhandled state: " << state_name.get_string().value() << '\n';
                    return 1;
                }

                initial_states.push_back(*state);
            }
        }

        std::optional<std::string> last_start_tag;
        if (test["lastStartTag"].error() == simdjson::SUCCESS) {
            last_start_tag = test["lastStartTag"].get_string().value();
        }

        auto in = test["input"].get_string().value();
        // TOOD(robinlinden): Don't skip these.
        // See: https://html.spec.whatwg.org/multipage/parsing.html#preprocessing-the-input-stream
        if (in.contains('\r')) {
            continue;
        }

        auto out_tokens = to_html2_tokens(test["output"].get_array().value());
        std::vector<Error> out_errors;

        if (test["errors"].error() == simdjson::SUCCESS) {
            auto maybe_errors = to_errors(test["errors"].get_array().value());
            if (!maybe_errors.has_value()) {
                continue;
            }

            out_errors = *std::move(maybe_errors);
        }

        for (auto state : initial_states) {
            auto test_name = std::string{name} + " (state: " + std::to_string(static_cast<int>(state)) + ")";
            s.add_test(std::move(test_name), [=, input = std::string{in}](auto &a) {
                auto [tokens, errors] = tokenize(input, state, last_start_tag);
                a.expect_eq(tokens, out_tokens);
                a.expect_eq(errors, out_errors);
            });
        }
    }

    return s.run();
}
// NOLINTEND(misc-include-cleaner)
