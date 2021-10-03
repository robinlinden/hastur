// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2021 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef CSS_PARSER_H_
#define CSS_PARSER_H_

#include "css/rule.h"

#include "util/base_parser.h"

#include <array>
#include <cstring>
#include <optional>
#include <stdexcept>
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
    static constexpr auto absolute_size_keywords =
            std::array{"xx-small", "x-small", "small", "medium", "large", "x-large", "xx-large", "xxx-large"};

    static constexpr auto relative_size_keywords = std::array{"larger", "smaller"};

    static constexpr auto weight_keywords = std::array{"bold", "bolder", "lighter"};

    static constexpr std::string_view dot_and_digits = ".0123456789";

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

        std::optional<std::string_view> peek() const {
            if (empty() || ((token_iter + 1) == cend(tokens))) {
                return std::nullopt;
            } else {
                return *(token_iter + 1);
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
            std::map<std::string, std::string> &declarations, std::string_view name, std::string_view value) const {
        if (name == "padding") {
            expand_padding(declarations, value);
        } else if (name == "font") {
            expand_font(declarations, value);
        } else {
            declarations.insert_or_assign(std::string{name}, std::string{value});
        }
    }

    void expand_padding(std::map<std::string, std::string> &declarations, std::string_view value) const {
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

    void expand_font(std::map<std::string, std::string> &declarations, std::string_view value) const {
        std::string_view font_stretch = "normal";
        std::string_view font_variant = "normal";
        std::string_view font_weight = "normal";
        std::string_view line_height = "normal";
        std::string font_style = "normal";

        Tokenizer tokenizer(value, ' ');
        if (tokenizer.size() == 1) {
            // TODO(mkiael): Handle system properties correctly. Just forward it for now.
            declarations.insert_or_assign("font", std::string{tokenizer.get().value()});
            return;
        }

        std::string_view font_size = "";
        std::string font_family = "";
        while (!tokenizer.empty()) {
            if (auto maybe_font_size = try_parse_font_size(tokenizer)) {
                auto [fs, lh] = *maybe_font_size;
                font_size = fs;
                line_height = lh.value_or(line_height);
                if (auto maybe_font_family = try_parse_font_family(tokenizer.next())) {
                    font_family = *maybe_font_family;
                }
                // Lets break here since font size and family should be last
                break;
            } else if (auto maybe_font_style = try_parse_font_style(tokenizer)) {
                font_style = *maybe_font_style;
            } else if (auto maybe_font_weight = try_parse_font_weight(tokenizer)) {
                font_weight = *maybe_font_weight;
            } else if (auto maybe_font_variant = try_parse_font_variant(tokenizer)) {
                font_variant = *maybe_font_variant;
            }
            // TODO(mkiael): Handle remaining properties
            tokenizer.next();
        }

        declarations.insert_or_assign("font-style", font_style);
        declarations.insert_or_assign("font-variant", std::string{font_variant});
        declarations.insert_or_assign("font-weight", std::string{font_weight});
        declarations.insert_or_assign("font-stretch", std::string{font_stretch});
        declarations.insert_or_assign("font-size", std::string{font_size});
        declarations.insert_or_assign("line-height", std::string{line_height});
        declarations.insert_or_assign("font-family", font_family);
    }

    std::optional<std::pair<std::string_view, std::optional<std::string_view>>> try_parse_font_size(
            Tokenizer &tokenizer) const {
        if (auto token = tokenizer.get()) {
            std::string_view str = *token;
            if (std::size_t loc = str.find('/'); loc != std::string_view::npos) {
                std::string_view font_size = str.substr(0, loc);
                std::string_view line_height = str.substr(loc + 1, str.size() - loc);
                return std::pair(std::move(font_size), std::move(line_height));
            } else if (is_absolute_size(str) || is_relative_size(str) || is_length_or_percentage(str)) {
                return std::pair(std::move(str), std::nullopt);
            }
        }
        return std::nullopt;
    }

    std::optional<std::string> try_parse_font_family(Tokenizer &tokenizer) const {
        std::string font_family = "";
        while (auto str = tokenizer.get()) {
            if (!font_family.empty()) {
                font_family += ' ';
            }
            font_family += *str;
            tokenizer.next();
        }
        return font_family;
    }

    std::optional<std::string> try_parse_font_style(Tokenizer &tokenizer) const {
        std::string font_style = "";
        if (auto maybe_font_style = tokenizer.get()) {
            if (maybe_font_style->starts_with("italic")) {
                font_style = *maybe_font_style;
                return font_style;
            } else if (maybe_font_style->starts_with("oblique")) {
                font_style = *maybe_font_style;
                if (auto maybe_angle = tokenizer.peek()) {
                    if (maybe_angle->find("deg") != std::string_view::npos) {
                        font_style += ' ';
                        font_style += *maybe_angle;
                        tokenizer.next();
                    }
                }
                return font_style;
            }
        }
        return std::nullopt;
    }

    std::optional<std::string_view> try_parse_font_weight(Tokenizer &tokenizer) const {
        if (auto maybe_font_weight = tokenizer.get()) {
            if (is_weight(*maybe_font_weight)) {
                return *maybe_font_weight;
            } else if (auto maybe_int = to_int(*maybe_font_weight); *maybe_int >= 1 && *maybe_int <= 1000) {
                return *maybe_font_weight;
            }
        }
        return std::nullopt;
    }

    std::optional<std::string_view> try_parse_font_variant(Tokenizer &tokenizer) const {
        if (auto maybe_font_variant = tokenizer.get()) {
            if (*maybe_font_variant == "small-caps") {
                return *maybe_font_variant;
            }
        }
        return std::nullopt;
    }

    std::optional<int> to_int(std::string_view str) const {
        try {
            return std::stoi(std::string{str});
        } catch (std::invalid_argument const &) {
            return std::nullopt;
        }
    }

    template<auto const &array>
    constexpr bool is_in_array(std::string_view str) const {
        return std::find(std::cbegin(array), std::cend(array), str) != std::cend(array);
    }

    constexpr bool is_absolute_size(std::string_view str) const { return is_in_array<absolute_size_keywords>(str); }

    constexpr bool is_relative_size(std::string_view str) const { return is_in_array<relative_size_keywords>(str); }

    constexpr bool is_weight(std::string_view str) const { return is_in_array<weight_keywords>(str); }

    constexpr bool is_length_or_percentage(std::string_view str) const {
        // TODO(mkiael): Make this check more reliable.
        std::size_t pos = str.find_first_not_of(dot_and_digits);
        return pos > 0 && pos != std::string_view::npos;
    }
};

} // namespace css

#endif
