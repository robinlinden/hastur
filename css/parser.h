// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2021 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef CSS_PARSER_H_
#define CSS_PARSER_H_

#include "css/property_id.h"
#include "css/rule.h"

#include "util/base_parser.h"

#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace css {

class Parser final : util::BaseParser {
public:
    explicit Parser(std::string_view input) : BaseParser{input} {}

    std::vector<css::Rule> parse_rules();

private:
    void skip_whitespace_and_comments();
    constexpr void skip_if_neq(char);

    css::Rule parse_rule();
    std::pair<std::string_view, std::string_view> parse_declaration();

    void add_declaration(
            std::map<PropertyId, std::string> &declarations, std::string_view name, std::string_view value) const;

    enum class BorderSide { Left, Right, Top, Bottom };

    // https://developer.mozilla.org/en-US/docs/Web/CSS/border
    void expand_border(
            std::string_view name, std::map<PropertyId, std::string> &declarations, std::string_view value) const;

    void expand_border_impl(BorderSide, std::map<PropertyId, std::string> &declarations, std::string_view value) const;

    // https://developer.mozilla.org/en-US/docs/Web/CSS/background
    // TODO(robinlinden): This only handles a color being named, and assumes any single item listed is a color.
    static void expand_background(std::map<PropertyId, std::string> &declarations, std::string_view value);

    // https://developer.mozilla.org/en-US/docs/Web/CSS/border-radius
    static void expand_border_radius_values(std::map<PropertyId, std::string> &declarations, std::string_view value);

    void expand_edge_values(
            std::map<PropertyId, std::string> &declarations, std::string property, std::string_view value) const;

    void expand_font(std::map<PropertyId, std::string> &declarations, std::string_view value) const;
};

inline std::vector<Rule> parse(std::string_view input) {
    return Parser{input}.parse_rules();
}

} // namespace css

#endif
