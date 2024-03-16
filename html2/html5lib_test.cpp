// SPDX-FileCopyrightText: 2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "html2/token.h"
#include "html2/tokenizer.h"

#include "etest/etest2.h"

#include <simdjson.h> // IWYU pragma: keep

#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace {
struct Error {
    html2::ParseError error{};
    html2::SourceLocation location{};
};

std::pair<std::vector<html2::Token>, std::vector<Error>> tokenize(std::string_view input) {
    std::vector<html2::Token> tokens;
    std::vector<Error> errors;
    html2::Tokenizer tokenizer{input,
            [&](html2::Tokenizer &t, html2::Token token) {
                // The expected token output doesn't contain eof tokens.
                if (std::holds_alternative<html2::EndOfFileToken>(token)) {
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
        if (test["initialStates"].error() == simdjson::SUCCESS) {
            continue;
        }
        auto in = test["input"].get_string().value();
        auto out_tokens = to_html2_tokens(test["output"].get_array().value());

        s.add_test(std::string{name}, [input = std::string{in}, expected = std::move(out_tokens)](etest::IActions &a) {
            auto [tokens, errors] = tokenize(input);
            a.expect_eq(tokens, expected);
            // TODO(robinlinden): Check that errors match.
        });
    }

    return s.run();
}
// NOLINTEND(misc-include-cleaner)
