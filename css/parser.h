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

    template<auto const &array>
    constexpr bool is_in_array(std::string_view) const;

    class Tokenizer;

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

    std::optional<std::pair<std::string_view, std::optional<std::string_view>>> try_parse_font_size(Tokenizer &) const;

    std::optional<std::string> try_parse_font_family(Tokenizer &) const;
    std::optional<std::string> try_parse_font_style(Tokenizer &) const;
    std::optional<std::string_view> try_parse_font_weight(Tokenizer &) const;
    std::optional<std::string_view> try_parse_font_variant(Tokenizer &) const;
    std::optional<std::string_view> try_parse_font_stretch(Tokenizer &) const;

    std::optional<int> to_int(std::string_view) const;

    constexpr bool is_shorthand_edge_property(std::string_view) const;
    constexpr bool is_absolute_size(std::string_view) const;
    constexpr bool is_relative_size(std::string_view) const;
    constexpr bool is_weight(std::string_view) const;
    constexpr bool is_stretch(std::string_view) const;
    constexpr bool is_length_or_percentage(std::string_view) const;
};

} // namespace css

#endif
