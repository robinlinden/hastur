// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef CSS_PARSER_H_
#define CSS_PARSER_H_

#include "css/rule.h"

#include "util/base_parser.h"

#include <cstring>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace css {

class Parser final : util::BaseParser {
public:
    Parser(std::string_view input) : BaseParser{input} {}

    std::vector<css::Rule> parse_rules() {
        std::vector<css::Rule> rules;
        std::string_view media_query;

        skip_whitespace();
        while (!is_eof()) {
            if (starts_with("@media ")) {
                advance(std::strlen("@media"));
                skip_whitespace();

                media_query = consume_while([](char c) { return c != '{'; });
                if (auto last_char = media_query.find_last_not_of(' '); last_char != std::string_view::npos) {
                    media_query.remove_suffix(media_query.size() - (last_char + 1));
                }
                consume_char(); // {
                skip_whitespace();
            }

            rules.push_back(parse_rule());
            if (!media_query.empty()) {
                rules.back().media_query = std::string{media_query};
            }

            skip_whitespace();

            if (!media_query.empty() && peek() == '}') {
                media_query = {};
                consume_char(); // }
                skip_whitespace();
            }
        }

        return rules;
    }

private:
    class Tokenizer {
    public:
        Tokenizer(std::string_view str, char delimiter) {
            std::size_t pos = 0, loc = 0;
            while ((loc = str.find(delimiter, pos)) != std::string_view::npos) {
                if (auto substr = str.substr(pos, loc - pos); !substr.empty()) {
                    tokens.push_back(substr);
                }
                pos = loc + 1;
            }
            if (pos < str.size()) {
                tokens.push_back(str.substr(pos, str.size() - pos));
            }
            token_iter = cbegin(tokens);
        }

        std::optional<std::string_view> get() const {
            if (empty()) {
                return std::nullopt;
            } else {
                return *token_iter;
            }
        }

        Tokenizer &next() {
            if (!empty()) {
                ++token_iter;
            }
            return *this;
        }

        bool empty() const { return token_iter == cend(tokens); }

        std::size_t size() const { return tokens.size(); }

    private:
        std::vector<std::string_view> tokens;
        std::vector<std::string_view>::const_iterator token_iter;
    };

    constexpr void skip_if_neq(char c) {
        if (peek() != c) {
            advance(1);
        }
    }

    css::Rule parse_rule() {
        Rule rule{};
        while (peek() != '{') {
            auto selector = consume_while([](char c) { return c != ' ' && c != ',' && c != '{'; });
            rule.selectors.push_back(std::string{selector});
            skip_if_neq('{'); // ' ' or ','
            skip_whitespace();
        }

        consume_char(); // {
        skip_whitespace();

        while (peek() != '}') {
            auto [name, value] = parse_declaration();
            add_declaration(rule.declarations, name, value);
            skip_whitespace();
        }

        consume_char(); // }

        return rule;
    }

    std::pair<std::string_view, std::string_view> parse_declaration() {
        auto name = consume_while([](char c) { return c != ':'; });
        consume_char(); // :
        skip_whitespace();
        auto value = consume_while([](char c) { return c != ';' && c != '}'; });
        skip_if_neq('}'); // ;
        return {name, value};
    }

    void add_declaration(
            std::map<std::string, std::string> &declarations, std::string_view name, std::string_view value) {
        if (name == "padding") {
            expand_padding(declarations, value);
        } else {
            declarations.insert_or_assign(std::string{name}, std::string{value});
        }
    }

    void expand_padding(std::map<std::string, std::string> &declarations, std::string_view value) {
        std::string_view top = "", bottom = "", left = "", right = "";
        Tokenizer tokenizer(value, ' ');
        switch (tokenizer.size()) {
            case 1:
                top = bottom = left = right = tokenizer.get().value();
                break;
            case 2:
                top = bottom = tokenizer.get().value();
                left = right = tokenizer.next().get().value();
                break;
            case 3:
                top = tokenizer.get().value();
                left = right = tokenizer.next().get().value();
                bottom = tokenizer.next().get().value();
                break;
            case 4:
                top = tokenizer.get().value();
                right = tokenizer.next().get().value();
                bottom = tokenizer.next().get().value();
                left = tokenizer.next().get().value();
                break;
            default:
                break;
        }
        declarations.insert_or_assign("padding-top", std::string{top});
        declarations.insert_or_assign("padding-bottom", std::string{bottom});
        declarations.insert_or_assign("padding-left", std::string{left});
        declarations.insert_or_assign("padding-right", std::string{right});
    }
};

} // namespace css

#endif
