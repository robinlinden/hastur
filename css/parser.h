#ifndef CSS_PARSER_H_
#define CSS_PARSER_H_

#include "css/rule.h"

#include "util/base_parser.h"

#include <string_view>
#include <utility>
#include <vector>

namespace css {

class Parser final : util::BaseParser {
public:
    Parser(std::string_view input) : BaseParser{input} {}

    std::vector<css::Rule> parse_rules() {
        std::vector<css::Rule> rules;

        skip_whitespace();
        while (!is_eof()) {
            rules.push_back(parse_rule());
            skip_whitespace();
        }

        return rules;
    }

private:
    css::Rule parse_rule() {
        Rule rule{};
        while (peek() != '{') {
            auto selector = consume_while([](char c) { return c != ' ' && c != ','; });
            rule.selectors.push_back(std::string{selector});
            consume_char(); // ' ' or ','
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
        auto value = consume_while([](char c) { return c != ';'; });
        consume_char(); // ;
        return {std::string{name}, std::string{value}};
    }
};

} // namespace css

#endif
