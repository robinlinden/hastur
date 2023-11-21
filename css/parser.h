// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2021 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef CSS_PARSER_H_
#define CSS_PARSER_H_

#include "css/property_id.h"
#include "css/rule.h"
#include "css/style_sheet.h"

#include <concepts>
#include <cstddef>
#include <optional>
#include <string_view>
#include <utility>

namespace css {

template<typename T>
concept Predicate = std::predicate<T, char>;

class Parser {
public:
    explicit Parser(std::string_view input) : input_{input} {}

    StyleSheet parse_rules();

private:
    std::string_view input_;
    std::size_t pos_{};

    // Parse helpers.
    constexpr bool is_eof() const { return pos_ >= input_.size(); }
    constexpr std::optional<char> peek() const;
    constexpr std::optional<std::string_view> peek(std::size_t) const;
    constexpr bool starts_with(std::string_view) const;
    constexpr void advance(std::size_t n) { pos_ += n; }
    constexpr void skip_if_neq(char);
    constexpr std::optional<char> consume_char();

    template<Predicate T>
    constexpr std::optional<std::string_view> consume_while(T const &pred);

    constexpr void skip_whitespace();

    // CSS-specific parsing bits.
    using Declarations = std::map<PropertyId, std::string>;

    void skip_whitespace_and_comments();

    std::optional<css::Rule> parse_rule();
    std::optional<std::pair<std::string_view, std::string_view>> parse_declaration();

    static void add_declaration(Declarations &, std::string_view name, std::string_view value);

    enum class BorderSide { Left, Right, Top, Bottom };

    // https://developer.mozilla.org/en-US/docs/Web/CSS/border
    static void expand_border(std::string_view name, Declarations &, std::string_view value);

    static void expand_border_impl(BorderSide, Declarations &, std::string_view value);

    // https://developer.mozilla.org/en-US/docs/Web/CSS/background
    // TODO(robinlinden): This only handles a color being named, and assumes any single item listed is a color.
    static void expand_background(Declarations &, std::string_view value);

    // https://developer.mozilla.org/en-US/docs/Web/CSS/border-radius
    static void expand_border_radius_values(Declarations &, std::string_view value);

    static void expand_text_decoration_values(Declarations &, std::string_view value);

    // https://developer.mozilla.org/en-US/docs/Web/CSS/flex-flow
    static void expand_flex_flow(Declarations &, std::string_view);

    static void expand_edge_values(Declarations &, std::string property, std::string_view value);

    static void expand_font(Declarations &, std::string_view value);
};

inline StyleSheet parse(std::string_view input) {
    return Parser{input}.parse_rules();
}

} // namespace css

#endif
