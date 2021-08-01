#ifndef CSS_PARSER_H_
#define CSS_PARSER_H_

#include "css/rule.h"

#include "util/base_parser.h"

#include <cstring>
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
    constexpr void skip_if_neq(char c) {
        if (peek() != c) { advance(1); }
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
            rule.declarations.insert(parse_declaration());
            skip_whitespace();
        }

        consume_char(); // }

        return rule;
    }

    std::pair<std::string, std::string> parse_declaration() {
        auto name = consume_while([](char c) { return c != ':'; });
        consume_char(); // :
        skip_whitespace();
        auto value = consume_while([](char c) { return c != ';' && c != '}'; });
        skip_if_neq('}'); // ;
        return {std::string{name}, std::string{value}};
    }
};

} // namespace css

#endif
