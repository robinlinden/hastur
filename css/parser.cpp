// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2021 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "css/parser.h"

#include "css/media_query.h"
#include "css/property_id.h"
#include "css/rule.h"

#include "util/string.h"

#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace css {
namespace {

constexpr std::array kBorderShorthandProperties{"border", "border-left", "border-right", "border-top", "border-bottom"};

// https://developer.mozilla.org/en-US/docs/Web/CSS/border-style
constexpr std::array kBorderStyleKeywords{
        "none", "hidden", "dotted", "dashed", "solid", "double", "groove", "ridge", "inset", "outset"};

// https://developer.mozilla.org/en-US/docs/Web/CSS/border-width
constexpr std::array kBorderWidthKeywords{"thin", "medium", "thick"};

constexpr auto kShorthandEdgeProperties = std::array{"padding", "margin", "border-style"};

constexpr auto kAbsoluteSizeKeywords =
        std::array{"xx-small", "x-small", "small", "medium", "large", "x-large", "xx-large", "xxx-large"};

constexpr auto kRelativeSizeKeywords = std::array{"larger", "smaller"};

constexpr auto kWeightKeywords = std::array{"bold", "bolder", "lighter"};

constexpr auto kStretchKeywords = std::array{"ultra-condensed",
        "extra-condensed",
        "condensed",
        "semi-condensed",
        "semi-expanded",
        "expanded",
        "extra-expanded",
        "ultra-expanded"};

constexpr std::string_view kDotAndDigits = ".0123456789";

template<auto const &array>
constexpr bool is_in_array(std::string_view str) {
    return std::ranges::find(array, str) != std::cend(array);
}

constexpr bool is_shorthand_edge_property(std::string_view str) {
    return is_in_array<kShorthandEdgeProperties>(str);
}

constexpr bool is_absolute_size(std::string_view str) {
    return is_in_array<kAbsoluteSizeKeywords>(str);
}

constexpr bool is_relative_size(std::string_view str) {
    return is_in_array<kRelativeSizeKeywords>(str);
}

constexpr bool is_weight(std::string_view str) {
    return is_in_array<kWeightKeywords>(str);
}

constexpr bool is_stretch(std::string_view str) {
    return is_in_array<kStretchKeywords>(str);
}

constexpr bool is_length_or_percentage(std::string_view str) {
    // TODO(mkiael): Make this check more reliable.
    std::size_t pos = str.find_first_not_of(kDotAndDigits);
    return pos > 0 && pos != std::string_view::npos;
}

std::optional<int> to_int(std::string_view str) {
    int result{};
    if (std::from_chars(str.data(), str.data() + str.size(), result).ec != std::errc{}) {
        return std::nullopt;
    }
    return result;
}

class Tokenizer {
public:
    Tokenizer(std::string_view str, char delimiter) {
        std::size_t pos = 0, loc = 0;
        while ((loc = str.find(delimiter, pos)) != std::string_view::npos) {
            if (auto substr = str.substr(pos, loc - pos); !substr.empty()) {
                tokens_.push_back(substr);
            }
            pos = loc + 1;
        }
        if (pos < str.size()) {
            tokens_.push_back(str.substr(pos));
        }
        token_iter_ = cbegin(tokens_);
    }

    std::optional<std::string_view> get() const {
        if (empty()) {
            return std::nullopt;
        } else {
            return *token_iter_;
        }
    }

    std::optional<std::string_view> peek() const {
        if (empty() || ((token_iter_ + 1) == cend(tokens_))) {
            return std::nullopt;
        } else {
            return *(token_iter_ + 1);
        }
    }

    Tokenizer &next() {
        if (!empty()) {
            ++token_iter_;
        }
        return *this;
    }

    bool empty() const { return token_iter_ == cend(tokens_); }

    std::size_t size() const { return tokens_.size(); }

private:
    std::vector<std::string_view> tokens_;
    std::vector<std::string_view>::const_iterator token_iter_;
};

std::optional<std::pair<std::string_view, std::optional<std::string_view>>> try_parse_font_size(Tokenizer &tokenizer) {
    if (auto token = tokenizer.get()) {
        std::string_view str = *token;
        if (std::size_t loc = str.find('/'); loc != std::string_view::npos) {
            std::string_view font_size = str.substr(0, loc);
            std::string_view line_height = str.substr(loc + 1);
            return std::pair(std::move(font_size), std::move(line_height));
        } else if (is_absolute_size(str) || is_relative_size(str) || is_length_or_percentage(str)) {
            return std::pair(std::move(str), std::nullopt);
        }
    }
    return std::nullopt;
}

std::optional<std::string> try_parse_font_family(Tokenizer &tokenizer) {
    std::string font_family;
    while (auto str = tokenizer.get()) {
        if (!font_family.empty()) {
            font_family += ' ';
        }
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access): False positive.
        font_family += *str;
        tokenizer.next();
    }
    return font_family;
}

std::optional<std::string> try_parse_font_style(Tokenizer &tokenizer) {
    std::string font_style;
    if (auto maybe_font_style = tokenizer.get()) {
        if (maybe_font_style->starts_with("italic")) {
            font_style = *maybe_font_style;
            return font_style;
        } else if (maybe_font_style->starts_with("oblique")) {
            font_style = *maybe_font_style;
            if (auto maybe_angle = tokenizer.peek()) {
                if (maybe_angle->contains("deg")) {
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

std::optional<std::string_view> try_parse_font_weight(Tokenizer &tokenizer) {
    if (auto maybe_font_weight = tokenizer.get()) {
        if (is_weight(*maybe_font_weight)) {
            return *maybe_font_weight;
        } else if (auto maybe_int = to_int(*maybe_font_weight)) {
            if (*maybe_int >= 1 && *maybe_int <= 1000) {
                return *maybe_font_weight;
            }
        }
    }
    return std::nullopt;
}

std::optional<std::string_view> try_parse_font_variant(Tokenizer &tokenizer) {
    if (auto maybe_font_variant = tokenizer.get()) {
        if (*maybe_font_variant == "small-caps") {
            return *maybe_font_variant;
        }
    }
    return std::nullopt;
}

std::optional<std::string_view> try_parse_font_stretch(Tokenizer &tokenizer) {
    if (auto maybe_font_stretch = tokenizer.get()) {
        if (is_stretch(*maybe_font_stretch)) {
            return *maybe_font_stretch;
        }
    }
    return std::nullopt;
}

} // namespace

// Not in header order, but must be defined before Parser::parse_rules() that
// uses this.
template<Predicate T>
constexpr std::optional<std::string_view> Parser::consume_while(T const &pred) {
    std::size_t start = pos_;
    while (!is_eof() && pred(input_[pos_])) {
        ++pos_;
    }

    if (is_eof()) {
        return std::nullopt;
    }

    return input_.substr(start, pos_ - start);
}

StyleSheet Parser::parse_rules() {
    StyleSheet style;
    bool in_media_query{false};
    std::optional<MediaQuery> media_query;

    skip_whitespace_and_comments();
    while (!is_eof()) {
        if (starts_with("@media ") || starts_with("@media(")) {
            advance(std::strlen("@media"));
            skip_whitespace_and_comments();

            auto tmp_query = consume_while([](char c) { return c != '{'; });
            if (!tmp_query) {
                spdlog::error("Eof while looking for end of media-query");
                return style;
            }

            if (auto last_char = tmp_query->find_last_not_of(' '); last_char != std::string_view::npos) {
                tmp_query->remove_suffix(tmp_query->size() - (last_char + 1));
            }
            in_media_query = true;
            media_query = MediaQuery::parse(*tmp_query);
            if (!media_query) {
                spdlog::warn("Unable to parse media query: '{}'", *tmp_query);
            }
            consume_char(); // {
            skip_whitespace_and_comments();
        }

        // Make sure we don't crash if we hit a currently unsupported at-rule.
        // @font-face works fine with the normal parsing-logic.
        if (starts_with("@") && !starts_with("@font-face")) {
            auto kind = consume_while([](char c) { return c != ' ' && c != '{' && c != '('; });
            if (!kind) {
                spdlog::error("Eof while looking for end of at-rule");
                return style;
            }

            spdlog::warn("Encountered unhandled {} at-rule", *kind);

            skip_whitespace_and_comments();
            std::ignore = consume_while([](char c) { return c != '{'; });
            consume_char(); // {
            skip_whitespace_and_comments();

            while (peek() != '}') {
                if (auto rule = parse_rule(); !rule) {
                    spdlog::error("Eof while looking for end of rule in unknown at-rule");
                    return style;
                }

                skip_whitespace_and_comments();
            }

            consume_char(); // }
            skip_whitespace_and_comments();
            continue;
        }

        auto rule = parse_rule();
        if (!rule) {
            spdlog::error("Eof while parsing rule");
            return style;
        }

        style.rules.push_back(*std::move(rule));
        style.rules.back().media_query = media_query;

        skip_whitespace_and_comments();

        if (in_media_query && peek() == '}') {
            media_query = {};
            in_media_query = false;
            consume_char(); // }
            skip_whitespace_and_comments();
        }
    }

    return style;
}

constexpr std::optional<char> Parser::peek() const {
    if (is_eof()) {
        return std::nullopt;
    }

    return input_[pos_];
}

constexpr std::optional<std::string_view> Parser::peek(std::size_t chars) const {
    if (is_eof()) {
        return std::nullopt;
    }

    return input_.substr(pos_, chars);
}

constexpr bool Parser::starts_with(std::string_view prefix) const {
    return peek(prefix.size()) == prefix;
}

constexpr void Parser::skip_if_neq(char c) {
    if (peek() != c) {
        advance(1);
    }
}

constexpr std::optional<char> Parser::consume_char() {
    if (is_eof()) {
        return std::nullopt;
    }

    return input_[pos_++];
}

constexpr void Parser::skip_whitespace() {
    for (auto c = peek(); c && util::is_whitespace(*c); c = peek()) {
        advance(1);
    }
}

void Parser::skip_whitespace_and_comments() {
    if (starts_with("/*")) {
        advance(2);
        consume_while([&](char) { return peek(2) != "*/"; });
        advance(2);
    }

    skip_whitespace();

    if (starts_with("/*")) {
        skip_whitespace_and_comments();
    }
}

std::optional<css::Rule> Parser::parse_rule() {
    Rule rule{};
    while (peek() != '{') {
        auto selector = consume_while([](char c) { return c != ',' && c != '{'; });
        if (!selector) {
            return std::nullopt;
        }

        rule.selectors.push_back(std::string{util::trim(*selector)});
        skip_if_neq('{'); // ' ' or ','
        skip_whitespace_and_comments();
    }

    consume_char(); // {
    skip_whitespace_and_comments();

    while (peek() != '}') {
        auto decl = parse_declaration();
        if (!decl) {
            return std::nullopt;
        }

        auto [name, value] = *decl;
        add_declaration(rule.declarations, name, value);
        skip_whitespace_and_comments();
    }

    consume_char(); // }

    return rule;
}

std::optional<std::pair<std::string_view, std::string_view>> Parser::parse_declaration() {
    auto name = consume_while([](char c) { return c != ':'; });
    if (!name) {
        return std::nullopt;
    }

    consume_char(); // :
    skip_whitespace_and_comments();
    auto value = consume_while([](char c) { return c != ';' && c != '}'; });
    if (!value) {
        return std::nullopt;
    }

    skip_if_neq('}'); // ;
    return std::pair{*name, *value};
}

void Parser::add_declaration(
        std::map<PropertyId, std::string> &declarations, std::string_view name, std::string_view value) const {
    if (is_shorthand_edge_property(name)) {
        expand_edge_values(declarations, std::string{name}, value);
    } else if (name == "background") {
        expand_background(declarations, value);
    } else if (name == "font") {
        expand_font(declarations, value);
    } else if (name == "border-radius") {
        expand_border_radius_values(declarations, value);
    } else if (name == "text-decoration") {
        expand_text_decoration_values(declarations, value);
    } else if (name == "flex-flow") {
        expand_flex_flow(declarations, value);
    } else if (is_in_array<kBorderShorthandProperties>(name)) {
        expand_border(name, declarations, value);
    } else {
        declarations.insert_or_assign(property_id_from_string(name), std::string{value});
    }
}

enum class BorderSide { Left, Right, Top, Bottom };

// https://developer.mozilla.org/en-US/docs/Web/CSS/border
void Parser::expand_border(
        std::string_view name, std::map<PropertyId, std::string> &declarations, std::string_view value) const {
    if (name == "border") {
        expand_border_impl(BorderSide::Left, declarations, value);
        expand_border_impl(BorderSide::Right, declarations, value);
        expand_border_impl(BorderSide::Top, declarations, value);
        expand_border_impl(BorderSide::Bottom, declarations, value);
    } else if (name == "border-left") {
        expand_border_impl(BorderSide::Left, declarations, value);
    } else if (name == "border-right") {
        expand_border_impl(BorderSide::Right, declarations, value);
    } else if (name == "border-top") {
        expand_border_impl(BorderSide::Top, declarations, value);
    } else if (name == "border-bottom") {
        expand_border_impl(BorderSide::Bottom, declarations, value);
    }
}

void Parser::expand_border_impl(
        BorderSide side, std::map<PropertyId, std::string> &declarations, std::string_view value) const {
    auto [color_id, style_id, width_id] = [&] {
        switch (side) {
            case BorderSide::Left:
                return std::tuple{
                        PropertyId::BorderLeftColor, PropertyId::BorderLeftStyle, PropertyId::BorderLeftWidth};
            case BorderSide::Right:
                return std::tuple{
                        PropertyId::BorderRightColor, PropertyId::BorderRightStyle, PropertyId::BorderRightWidth};
            case BorderSide::Top:
                return std::tuple{PropertyId::BorderTopColor, PropertyId::BorderTopStyle, PropertyId::BorderTopWidth};
            case BorderSide::Bottom:
                return std::tuple{
                        PropertyId::BorderBottomColor, PropertyId::BorderBottomStyle, PropertyId::BorderBottomWidth};
        }
        std::abort(); // Unreachable.
    }();

    Tokenizer tokenizer(value, ' ');
    if (tokenizer.empty() || tokenizer.size() > 3) {
        // TODO(robinlinden): Propagate info about invalid properties.
        return;
    }

    enum class BorderPropertyType { Color, Style, Width };
    auto guess_type = [](std::string_view v) -> BorderPropertyType {
        if (is_in_array<kBorderStyleKeywords>(v)) {
            return BorderPropertyType::Style;
        }

        if (v.find_first_of(kDotAndDigits) == 0 || is_in_array<kBorderWidthKeywords>(v)) {
            return BorderPropertyType::Width;
        }

        return BorderPropertyType::Color;
    };

    std::optional<std::string_view> color;
    std::optional<std::string_view> style;
    std::optional<std::string_view> width;

    // TODO(robinlinden): Duplicate color/style/width shouldn't be
    // tolerated, but we have no way of propagating that info right now.
    // NOLINTBEGIN(bugprone-unchecked-optional-access): False positives.
    for (auto v = tokenizer.get(); v.has_value(); v = tokenizer.next().get()) {
        switch (guess_type(*v)) {
            case BorderPropertyType::Color:
                color = *v;
                break;
            case BorderPropertyType::Style:
                style = *v;
                break;
            case BorderPropertyType::Width:
                width = *v;
                break;
        }
    }
    // NOLINTEND(bugprone-unchecked-optional-access)

    declarations.insert_or_assign(color_id, color.value_or("currentcolor"));
    declarations.insert_or_assign(style_id, style.value_or("none"));
    declarations.insert_or_assign(width_id, width.value_or("medium"));
}

// https://developer.mozilla.org/en-US/docs/Web/CSS/background
// TODO(robinlinden): This only handles a color being named, and assumes any single item listed is a color.
void Parser::expand_background(std::map<PropertyId, std::string> &declarations, std::string_view value) {
    declarations[PropertyId::BackgroundImage] = "none";
    declarations[PropertyId::BackgroundPosition] = "0% 0%";
    declarations[PropertyId::BackgroundSize] = "auto auto";
    declarations[PropertyId::BackgroundRepeat] = "repeat";
    declarations[PropertyId::BackgroundOrigin] = "padding-box";
    declarations[PropertyId::BackgroundClip] = "border-box";
    declarations[PropertyId::BackgroundAttachment] = "scroll";
    declarations[PropertyId::BackgroundColor] = "transparent";

    Tokenizer tokenizer{value, ' '};
    if (tokenizer.size() == 1) {
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access): False positive.
        declarations[PropertyId::BackgroundColor] = tokenizer.get().value();
    }
}

// https://developer.mozilla.org/en-US/docs/Web/CSS/border-radius
void Parser::expand_border_radius_values(std::map<PropertyId, std::string> &declarations, std::string_view value) {
    std::string top_left, top_right, bottom_right, bottom_left;
    auto [horizontal, vertical] = util::split_once(value, "/");
    Tokenizer tokenizer(horizontal, ' ');
    // NOLINTBEGIN(bugprone-unchecked-optional-access): False positives.
    switch (tokenizer.size()) {
        case 1:
            top_left = top_right = bottom_right = bottom_left = tokenizer.get().value();
            break;
        case 2:
            top_left = bottom_right = tokenizer.get().value();
            top_right = bottom_left = tokenizer.next().get().value();
            break;
        case 3:
            top_left = tokenizer.get().value();
            top_right = bottom_left = tokenizer.next().get().value();
            bottom_right = tokenizer.next().get().value();
            break;
        case 4:
            top_left = tokenizer.get().value();
            top_right = tokenizer.next().get().value();
            bottom_right = tokenizer.next().get().value();
            bottom_left = tokenizer.next().get().value();
            break;
        default:
            break;
    }

    if (!vertical.empty()) {
        tokenizer = Tokenizer{vertical, ' '};
        switch (tokenizer.size()) {
            case 1: {
                auto v_radius{tokenizer.get().value()};
                top_left += fmt::format(" / {}", v_radius);
                top_right += fmt::format(" / {}", v_radius);
                bottom_right += fmt::format(" / {}", v_radius);
                bottom_left += fmt::format(" / {}", v_radius);
                break;
            }
            case 2: {
                auto v1_radius{tokenizer.get().value()};
                top_left += fmt::format(" / {}", v1_radius);
                bottom_right += fmt::format(" / {}", v1_radius);
                auto v2_radius{tokenizer.next().get().value()};
                top_right += fmt::format(" / {}", v2_radius);
                bottom_left += fmt::format(" / {}", v2_radius);
                break;
            }
            case 3: {
                top_left += fmt::format(" / {}", tokenizer.get().value());
                auto v_radius = tokenizer.next().get().value();
                top_right += fmt::format(" / {}", v_radius);
                bottom_left += fmt::format(" / {}", v_radius);
                bottom_right += fmt::format(" / {}", tokenizer.next().get().value());
                break;
            }
            case 4: {
                top_left += fmt::format(" / {}", tokenizer.get().value());
                top_right += fmt::format(" / {}", tokenizer.next().get().value());
                bottom_right += fmt::format(" / {}", tokenizer.next().get().value());
                bottom_left += fmt::format(" / {}", tokenizer.next().get().value());
                break;
            }
            default:
                break;
        }
    }
    // NOLINTEND(bugprone-unchecked-optional-access)

    declarations.insert_or_assign(PropertyId::BorderTopLeftRadius, top_left);
    declarations.insert_or_assign(PropertyId::BorderTopRightRadius, top_right);
    declarations.insert_or_assign(PropertyId::BorderBottomRightRadius, bottom_right);
    declarations.insert_or_assign(PropertyId::BorderBottomLeftRadius, bottom_left);
}

// https://drafts.csswg.org/css-text-decor/#text-decoration-property
void Parser::expand_text_decoration_values(std::map<PropertyId, std::string> &declarations, std::string_view value) {
    Tokenizer tokenizer{value, ' '};
    // TODO(robinlinden): CSS level 3 text-decorations.
    if (tokenizer.size() != 1) {
        spdlog::warn("Unsupported text-decoration value: '{}'", value);
        return;
    }

    declarations.insert_or_assign(PropertyId::TextDecorationColor, "currentcolor");
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access): False positive.
    declarations.insert_or_assign(PropertyId::TextDecorationLine, tokenizer.get().value());
    declarations.insert_or_assign(PropertyId::TextDecorationStyle, "solid");
}

// https://developer.mozilla.org/en-US/docs/Web/CSS/flex-flow
void Parser::expand_flex_flow(std::map<PropertyId, std::string> &declarations, std::string_view value) {
    static constexpr std::array kGlobalValues{"inherit", "initial", "revert", "revert-layer", "unset"};

    auto is_wrap = [](std::string_view str) {
        return str == "wrap" || str == "nowrap" || str == "wrap-reverse";
    };
    auto is_direction = [](std::string_view str) {
        return str == "row" || str == "row-reverse" || str == "column" || str == "column-reverse";
    };

    std::string direction{"row"};
    std::string wrap{"nowrap"};

    Tokenizer tokenizer{value, ' '};
    if (tokenizer.size() != 1 && tokenizer.size() != 2) {
        spdlog::warn("Unsupported flex-flow value: '{}'", value);
        return;
    }

    auto first = tokenizer.get();
    auto second = tokenizer.next().get();
    // Global values are only allowed if there's a single value.
    if (first && !second && is_in_array<kGlobalValues>(*first)) {
        wrap = direction = *first;
        declarations.insert_or_assign(PropertyId::FlexDirection, std::move(direction));
        declarations.insert_or_assign(PropertyId::FlexWrap, std::move(wrap));
        return;
    }

    // No duplicates of wrap or direction allowed.
    if ((first && second)
            && ((is_wrap(*first) && !is_direction(*second)) || (is_direction(*first) && !is_wrap(*second)))) {
        spdlog::warn("Unsupported flex-flow value: '{}'", value);
        return;
    }

    for (auto const &v : std::array{first, second}) {
        if (!v) {
            continue;
        }

        if (is_wrap(*v)) {
            wrap = *v;
        } else if (is_direction(*v)) {
            direction = *v;
        } else {
            spdlog::warn("Unsupported flex-flow value: '{}'", value);
            return;
        }
    }

    declarations.insert_or_assign(PropertyId::FlexDirection, std::move(direction));
    declarations.insert_or_assign(PropertyId::FlexWrap, std::move(wrap));
}

void Parser::expand_edge_values(
        std::map<PropertyId, std::string> &declarations, std::string property, std::string_view value) const {
    std::string_view top, bottom, left, right;
    Tokenizer tokenizer(value, ' ');
    // NOLINTBEGIN(bugprone-unchecked-optional-access): False positives.
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
    // NOLINTEND(bugprone-unchecked-optional-access)
    std::string post_fix;
    if (property == "border-style") {
        // border-style is a bit special as we want border-top-style instead of border-style-top-style.
        property = "border";
        post_fix = "-style";
    }
    declarations.insert_or_assign(
            property_id_from_string(fmt::format("{}-top{}", property, post_fix)), std::string{top});
    declarations.insert_or_assign(
            property_id_from_string(fmt::format("{}-bottom{}", property, post_fix)), std::string{bottom});
    declarations.insert_or_assign(
            property_id_from_string(fmt::format("{}-left{}", property, post_fix)), std::string{left});
    declarations.insert_or_assign(
            property_id_from_string(fmt::format("{}-right{}", property, post_fix)), std::string{right});
}

void Parser::expand_font(std::map<PropertyId, std::string> &declarations, std::string_view value) const {
    Tokenizer tokenizer(value, ' ');
    if (tokenizer.size() == 1) {
        // TODO(mkiael): Handle system properties correctly. Just forward it for now.
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access): False positive.
        declarations.insert_or_assign(PropertyId::FontFamily, std::string{tokenizer.get().value()});
        return;
    }

    std::string font_family;
    std::string font_style = "normal";
    std::string_view font_size;
    std::string_view font_stretch = "normal";
    std::string_view font_variant = "normal";
    std::string_view font_weight = "normal";
    std::string_view line_height = "normal";
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
        } else if (auto maybe_font_stretch = try_parse_font_stretch(tokenizer)) {
            font_stretch = *maybe_font_stretch;
        }
        tokenizer.next();
    }

    declarations.insert_or_assign(PropertyId::FontStyle, font_style);
    declarations.insert_or_assign(PropertyId::FontVariant, std::string{font_variant});
    declarations.insert_or_assign(PropertyId::FontWeight, std::string{font_weight});
    declarations.insert_or_assign(PropertyId::FontStretch, std::string{font_stretch});
    declarations.insert_or_assign(PropertyId::FontSize, std::string{font_size});
    declarations.insert_or_assign(PropertyId::LineHeight, std::string{line_height});
    declarations.insert_or_assign(PropertyId::FontFamily, font_family);

    // Reset all values that can't be specified in shorthand
    declarations.insert_or_assign(PropertyId::FontFeatureSettings, "normal");
    declarations.insert_or_assign(PropertyId::FontKerning, "auto");
    declarations.insert_or_assign(PropertyId::FontLanguageOverride, "normal");
    declarations.insert_or_assign(PropertyId::FontOpticalSizing, "auto");
    declarations.insert_or_assign(PropertyId::FontPalette, "normal");
    declarations.insert_or_assign(PropertyId::FontSizeAdjust, "none");
    declarations.insert_or_assign(PropertyId::FontVariationSettings, "normal");
    declarations.insert_or_assign(PropertyId::FontVariantAlternatives, "normal");
    declarations.insert_or_assign(PropertyId::FontVariantCaps, "normal");
    declarations.insert_or_assign(PropertyId::FontVariantLigatures, "normal");
    declarations.insert_or_assign(PropertyId::FontVariantNumeric, "normal");
    declarations.insert_or_assign(PropertyId::FontVariantPosition, "normal");
    declarations.insert_or_assign(PropertyId::FontVariantEastAsian, "normal");
}

} // namespace css
