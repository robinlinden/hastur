// SPDX-FileCopyrightText: 2021-2025 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2021 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "css/parser.h"

#include "css/media_query.h"
#include "css/property_id.h"
#include "css/rule.h"
#include "css/style_sheet.h"

#include "util/from_chars.h"
#include "util/string.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <charconv>
#include <concepts>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <format>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
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

constexpr auto kShorthandEdgeProperties = std::array{
        "padding",
        "margin",
        "border-color",
        "border-style",
        "border-width",
};

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

constexpr std::array kGlobalValues{"inherit", "initial", "revert", "revert-layer", "unset"};

constexpr bool is_shorthand_edge_property(std::string_view str) {
    return std::ranges::contains(kShorthandEdgeProperties, str);
}

constexpr bool is_absolute_size(std::string_view str) {
    return std::ranges::contains(kAbsoluteSizeKeywords, str);
}

constexpr bool is_relative_size(std::string_view str) {
    return std::ranges::contains(kRelativeSizeKeywords, str);
}

constexpr bool is_weight(std::string_view str) {
    return std::ranges::contains(kWeightKeywords, str);
}

constexpr bool is_stretch(std::string_view str) {
    return std::ranges::contains(kStretchKeywords, str);
}

bool is_length_or_percentage(std::string_view str) {
    // https://developer.mozilla.org/en-US/docs/Web/CSS/length
    static constexpr auto kLengthUnits = std::to_array({
            // Relative units based on font.
            "cap",
            "ch",
            "em",
            "ex",
            "ic",
            "lh",

            // Relative units based on root element's font.
            "rcap",
            "rch",
            "rem",
            "rex",
            "ric",
            "rlh",

            // Relative untis based on viewport.
            "vh",
            "vw",
            "vmax",
            "vmin",
            "vb",
            "vi",

            // Container query units.
            "cqw",
            "cqh",
            "cqi",
            "cqb",
            "cqmin",
            "cqmax",

            // Absolute units.
            "px",
            "cm",
            "mm",
            "Q",
            "in",
            "pc",
            "pt",
    });

    double d{};
    auto res = util::from_chars(str.data(), str.data() + str.size(), d);
    if (res.ec != std::errc{}) {
        return false;
    }

    auto unit = str.substr(std::distance(str.data(), res.ptr));
    return unit == "%" || std::ranges::find(kLengthUnits, unit) != std::cend(kLengthUnits);
}

std::optional<int> to_int(std::string_view str) {
    int result{};
    auto res = std::from_chars(str.data(), str.data() + str.size(), result);
    if (res.ec != std::errc{} || res.ptr != str.data() + str.size()) {
        return std::nullopt;
    }
    return result;
}

class Tokenizer {
public:
    Tokenizer(std::string_view str, char delimiter) {
        std::size_t pos = 0;
        std::size_t loc = 0;
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
        }

        return *token_iter_;
    }

    std::optional<std::string_view> peek() const {
        if (empty() || ((token_iter_ + 1) == cend(tokens_))) {
            return std::nullopt;
        }

        return *(token_iter_ + 1);
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
        }

        if (is_absolute_size(str) || is_relative_size(str) || is_length_or_percentage(str)) {
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
        }

        if (maybe_font_style->starts_with("oblique")) {
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
            return maybe_font_weight;
        }

        if (auto maybe_int = to_int(*maybe_font_weight)) {
            if (*maybe_int >= 1 && *maybe_int <= 1000) {
                return maybe_font_weight;
            }
        }
    }
    return std::nullopt;
}

std::optional<std::string_view> try_parse_font_variant(Tokenizer &tokenizer) {
    if (auto maybe_font_variant = tokenizer.get()) {
        if (*maybe_font_variant == "small-caps") {
            return maybe_font_variant;
        }
    }
    return std::nullopt;
}

std::optional<std::string_view> try_parse_font_stretch(Tokenizer &tokenizer) {
    if (auto maybe_font_stretch = tokenizer.get()) {
        if (is_stretch(*maybe_font_stretch)) {
            return maybe_font_stretch;
        }
    }
    return std::nullopt;
}

} // namespace

// Not in header order, but must be defined before Parser::parse_rules() that
// uses this.
constexpr std::optional<std::string_view> Parser::consume_while(std::predicate<char> auto const &pred) {
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
        if (starts_with("@charset ")) {
            advance(std::strlen("@charset"));
            skip_whitespace_and_comments();
            if (auto charset = consume_while([](char c) { return c != ';'; }); charset) {
                spdlog::warn("Ignoring charset: {}", *charset);
            } else {
                spdlog::error("Eof while parsing charset");
                return style;
            }

            consume_char(); // ;
            skip_whitespace_and_comments();
            continue;
        }

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
                media_query = MediaQuery{MediaQuery::False{}};
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

            if (kind == "@import") {
                std::ignore = consume_while([](char c) { return c != ';'; });
                consume_char(); // ;
                skip_whitespace_and_comments();
                spdlog::warn("Encountered unhandled import at-rule", *kind);
                continue;
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
    skip_whitespace();
    while (starts_with("/*")) {
        advance(2);
        consume_while([&](char) { return peek(2) != "*/"; });
        advance(2);
        skip_whitespace();
    }
}

// NOLINTNEXTLINE(misc-no-recursion)
std::optional<css::Rule> Parser::parse_rule() {
    Rule rule{};
    while (peek() != '{') {
        auto selector = consume_while([](char c) { return c != ',' && c != '{'; });
        if (!selector) {
            return std::nullopt;
        }

        rule.selectors.emplace_back(util::trim(*selector));
        skip_if_neq('{'); // ' ' or ','
        skip_whitespace_and_comments();
    }

    consume_char(); // {
    skip_whitespace_and_comments();

    while (peek() != '}') {
        // TODO(robinlinden): This doesn't get along with nested rules like
        // `foo { bar:baz { font-size: 3em; } }`
        // due to the assumption that "ascii:" always is a CSS property name.
        auto nested_rule_or_declaration_name = consume_while([](char c) { return c != ':' && c != '{'; });
        if (!nested_rule_or_declaration_name || nested_rule_or_declaration_name->empty()) {
            return std::nullopt;
        }

        // If a name starts w/ any of these, it's likely a nested rule w/ : as
        // part of the selector, e.g. &:hover { ... }. This isn't great, but
        // we're dropping this parser in favour of the css2 one soon(tm).
        if (peek() == '{' || std::string_view{".#>&[|+~:"}.contains(nested_rule_or_declaration_name->front())) {
            // TODO(robinlinden): Nested rule. Skip over it for now.
            pos_ -= nested_rule_or_declaration_name->size();
            if (auto nested_rule = parse_rule()) {
                spdlog::warn("Ignoring nested rule: '{}'", to_string(*nested_rule));
            } else {
                spdlog::warn("Unable to parse nested rule: '{}'", *nested_rule_or_declaration_name);
            }
            skip_whitespace_and_comments();
            continue;
        }

        auto decl = parse_declaration(*nested_rule_or_declaration_name);
        if (!decl) {
            return std::nullopt;
        }

        auto [name, value] = *decl;
        value = util::trim(value);
        if (name.starts_with("--")) {
            rule.custom_properties.insert_or_assign(std::string{name}, value);
        } else if (auto name_start_byte = name.front(); name_start_byte == '-') {
            // We don't really care about the -moz, -ms, -webkit, or similar prefixed properties.
            spdlog::debug("Ignoring vendor-prefixed property: '{}'", name);
        } else if (!util::is_alpha(name_start_byte)) {
            spdlog::warn("Ignoring unknown property: '{}'", name);
        } else if (value.ends_with("!important")) {
            value.remove_suffix(std::strlen("!important"));
            add_declaration(rule.important_declarations, name, util::trim(value));
        } else {
            add_declaration(rule.declarations, name, value);
        }
        skip_whitespace_and_comments();
    }

    consume_char(); // }

    return rule;
}

std::optional<std::pair<std::string_view, std::string_view>> Parser::parse_declaration(std::string_view name) {
    consume_char(); // :
    skip_whitespace_and_comments();
    auto value = consume_while([](char c) { return c != ';' && c != '}'; });
    if (!value) {
        return std::nullopt;
    }

    skip_if_neq('}'); // ;
    return std::pair{name, *value};
}

void Parser::add_declaration(Declarations &declarations, std::string_view name, std::string_view value) {
    if (is_shorthand_edge_property(name)) {
        expand_edge_values(declarations, name, value);
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
    } else if (std::ranges::contains(kBorderShorthandProperties, name)) {
        expand_border(name, declarations, value);
    } else if (name == "outline") {
        expand_outline(declarations, value);
    } else {
        declarations.insert_or_assign(property_id_from_string(name), std::string{value});
    }
}

// https://developer.mozilla.org/en-US/docs/Web/CSS/border
void Parser::expand_border(std::string_view name, Declarations &declarations, std::string_view value) {
    enum class BorderSide : std::uint8_t { Left, Right, Top, Bottom };

    static constexpr auto kIdsFor = [](BorderSide side) -> BorderOrOutlinePropertyIds {
        switch (side) {
            case BorderSide::Left:
                return BorderOrOutlinePropertyIds{
                        PropertyId::BorderLeftColor, PropertyId::BorderLeftStyle, PropertyId::BorderLeftWidth};
            case BorderSide::Right:
                return BorderOrOutlinePropertyIds{
                        PropertyId::BorderRightColor, PropertyId::BorderRightStyle, PropertyId::BorderRightWidth};
            case BorderSide::Top:
                return BorderOrOutlinePropertyIds{
                        PropertyId::BorderTopColor, PropertyId::BorderTopStyle, PropertyId::BorderTopWidth};
            case BorderSide::Bottom:
            default:
                return BorderOrOutlinePropertyIds{
                        PropertyId::BorderBottomColor, PropertyId::BorderBottomStyle, PropertyId::BorderBottomWidth};
        }
    };

    if (name == "border") {
        expand_border_or_outline_impl(kIdsFor(BorderSide::Left), declarations, value);
        expand_border_or_outline_impl(kIdsFor(BorderSide::Right), declarations, value);
        expand_border_or_outline_impl(kIdsFor(BorderSide::Top), declarations, value);
        expand_border_or_outline_impl(kIdsFor(BorderSide::Bottom), declarations, value);
    } else if (name == "border-left") {
        expand_border_or_outline_impl(kIdsFor(BorderSide::Left), declarations, value);
    } else if (name == "border-right") {
        expand_border_or_outline_impl(kIdsFor(BorderSide::Right), declarations, value);
    } else if (name == "border-top") {
        expand_border_or_outline_impl(kIdsFor(BorderSide::Top), declarations, value);
    } else if (name == "border-bottom") {
        expand_border_or_outline_impl(kIdsFor(BorderSide::Bottom), declarations, value);
    }
}

void Parser::expand_border_or_outline_impl(
        BorderOrOutlinePropertyIds ids, Declarations &declarations, std::string_view value) {
    Tokenizer tokenizer(value, ' ');
    if (tokenizer.empty() || tokenizer.size() > 3) {
        // TODO(robinlinden): Propagate info about invalid properties.
        return;
    }

    enum class BorderPropertyType : std::uint8_t { Color, Style, Width };
    auto guess_type = [](std::string_view v) -> BorderPropertyType {
        if (std::ranges::contains(kBorderStyleKeywords, v)) {
            return BorderPropertyType::Style;
        }

        if (v.find_first_of(kDotAndDigits) == 0 || std::ranges::contains(kBorderWidthKeywords, v)) {
            return BorderPropertyType::Width;
        }

        return BorderPropertyType::Color;
    };

    std::optional<std::string_view> color;
    std::optional<std::string_view> style;
    std::optional<std::string_view> width;

    // TODO(robinlinden): Duplicate color/style/width shouldn't be
    // tolerated, but we have no way of propagating that info right now.
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

    declarations.insert_or_assign(ids.color, color.value_or("currentcolor"));
    declarations.insert_or_assign(ids.style, style.value_or("none"));
    declarations.insert_or_assign(ids.width, width.value_or("medium"));
}

// https://developer.mozilla.org/en-US/docs/Web/CSS/outline
void Parser::expand_outline(Declarations &declarations, std::string_view outline) {
    static constexpr BorderOrOutlinePropertyIds kIds{
            PropertyId::OutlineColor, PropertyId::OutlineStyle, PropertyId::OutlineWidth};
    expand_border_or_outline_impl(kIds, declarations, outline);
}

// https://developer.mozilla.org/en-US/docs/Web/CSS/background
// TODO(robinlinden): This only handles a color being named, and assumes any single item listed is a color.
void Parser::expand_background(Declarations &declarations, std::string_view value) {
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
        auto bg_color = tokenizer.get();
        assert(bg_color.has_value());
        declarations[PropertyId::BackgroundColor] = *bg_color;
    }
}

// https://developer.mozilla.org/en-US/docs/Web/CSS/border-radius
void Parser::expand_border_radius_values(Declarations &declarations, std::string_view value) {
    std::string top_left;
    std::string top_right;
    std::string bottom_right;
    std::string bottom_left;
    auto [horizontal, vertical] = util::split_once(value, '/');
    Tokenizer tokenizer(horizontal, ' ');

    auto unchecked_get = [](Tokenizer &t) -> std::string_view {
        auto v = t.get();
        assert(v.has_value());
        return *v;
    };

    switch (tokenizer.size()) {
        case 1:
            top_left = top_right = bottom_right = bottom_left = unchecked_get(tokenizer);
            break;
        case 2:
            top_left = bottom_right = unchecked_get(tokenizer);
            top_right = bottom_left = unchecked_get(tokenizer.next());
            break;
        case 3:
            top_left = unchecked_get(tokenizer);
            top_right = bottom_left = unchecked_get(tokenizer.next());
            bottom_right = unchecked_get(tokenizer.next());
            break;
        case 4:
            top_left = unchecked_get(tokenizer);
            top_right = unchecked_get(tokenizer.next());
            bottom_right = unchecked_get(tokenizer.next());
            bottom_left = unchecked_get(tokenizer.next());
            break;
        default:
            break;
    }

    if (!vertical.empty()) {
        tokenizer = Tokenizer{vertical, ' '};
        switch (tokenizer.size()) {
            case 1: {
                auto v_radius{unchecked_get(tokenizer)};
                top_left += std::format(" / {}", v_radius);
                top_right += std::format(" / {}", v_radius);
                bottom_right += std::format(" / {}", v_radius);
                bottom_left += std::format(" / {}", v_radius);
                break;
            }
            case 2: {
                auto v1_radius{unchecked_get(tokenizer)};
                top_left += std::format(" / {}", v1_radius);
                bottom_right += std::format(" / {}", v1_radius);
                auto v2_radius{unchecked_get(tokenizer.next())};
                top_right += std::format(" / {}", v2_radius);
                bottom_left += std::format(" / {}", v2_radius);
                break;
            }
            case 3: {
                top_left += std::format(" / {}", unchecked_get(tokenizer));
                auto v_radius = unchecked_get(tokenizer.next());
                top_right += std::format(" / {}", v_radius);
                bottom_left += std::format(" / {}", v_radius);
                bottom_right += std::format(" / {}", unchecked_get(tokenizer.next()));
                break;
            }
            case 4: {
                top_left += std::format(" / {}", unchecked_get(tokenizer));
                top_right += std::format(" / {}", unchecked_get(tokenizer.next()));
                bottom_right += std::format(" / {}", unchecked_get(tokenizer.next()));
                bottom_left += std::format(" / {}", unchecked_get(tokenizer.next()));
                break;
            }
            default:
                break;
        }
    }

    declarations.insert_or_assign(PropertyId::BorderTopLeftRadius, top_left);
    declarations.insert_or_assign(PropertyId::BorderTopRightRadius, top_right);
    declarations.insert_or_assign(PropertyId::BorderBottomRightRadius, bottom_right);
    declarations.insert_or_assign(PropertyId::BorderBottomLeftRadius, bottom_left);
}

// https://drafts.csswg.org/css-text-decor/#text-decoration-property
// https://developer.mozilla.org/en-US/docs/Web/CSS/text-decoration
void Parser::expand_text_decoration_values(Declarations &declarations, std::string_view value) {
    // TODO(robinlinden): text-decoration-color, text-decoration-thickness.
    if (std::ranges::contains(kGlobalValues, value)) {
        declarations.insert_or_assign(PropertyId::TextDecorationColor, value);
        declarations.insert_or_assign(PropertyId::TextDecorationLine, value);
        declarations.insert_or_assign(PropertyId::TextDecorationStyle, value);
        return;
    }

    static constexpr std::array kTextDecorationLineKeywords{"none", "underline", "overline", "line-through", "blink"};
    static constexpr std::array kTextDecorationStyleKeywords{"solid", "double", "dotted", "dashed", "wavy"};

    std::optional<std::string_view> line;
    std::optional<std::string_view> style;

    Tokenizer tokenizer{value, ' '};
    for (auto v = tokenizer.get(); v.has_value(); v = tokenizer.next().get()) {
        if (std::ranges::contains(kTextDecorationLineKeywords, *v) && !line.has_value()) {
            line = *v;
        } else if (std::ranges::contains(kTextDecorationStyleKeywords, *v) && !style.has_value()) {
            style = *v;
        } else {
            spdlog::warn("Unsupported text-decoration value: '{}'", value);
            return;
        }
    }

    declarations.insert_or_assign(PropertyId::TextDecorationColor, "currentcolor");
    declarations.insert_or_assign(PropertyId::TextDecorationLine, line.value_or("none"));
    declarations.insert_or_assign(PropertyId::TextDecorationStyle, style.value_or("solid"));
}

// https://developer.mozilla.org/en-US/docs/Web/CSS/flex-flow
void Parser::expand_flex_flow(Declarations &declarations, std::string_view value) {
    auto is_wrap = [](std::string_view str) {
        return str == "wrap" || str == "nowrap" || str == "wrap-reverse";
    };
    auto is_direction = [](std::string_view str) {
        return str == "row" || str == "row-reverse" || str == "column" || str == "column-reverse";
    };

    std::string_view direction{"row"};
    std::string_view wrap{"nowrap"};

    Tokenizer tokenizer{value, ' '};
    if (tokenizer.size() != 1 && tokenizer.size() != 2) {
        spdlog::warn("Unsupported flex-flow value: '{}'", value);
        return;
    }

    auto first = tokenizer.get();
    auto second = tokenizer.next().get();
    // Global values are only allowed if there's a single value.
    if (first && !second && std::ranges::contains(kGlobalValues, *first)) {
        wrap = direction = *first;
        declarations.insert_or_assign(PropertyId::FlexDirection, direction);
        declarations.insert_or_assign(PropertyId::FlexWrap, wrap);
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

    declarations.insert_or_assign(PropertyId::FlexDirection, direction);
    declarations.insert_or_assign(PropertyId::FlexWrap, wrap);
}

void Parser::expand_edge_values(Declarations &declarations, std::string_view property, std::string_view value) {
    std::string_view top;
    std::string_view bottom;
    std::string_view left;
    std::string_view right;
    Tokenizer tokenizer(value, ' ');
    switch (tokenizer.size()) {
        case 1: {
            auto all = tokenizer.get();
            assert(all.has_value());
            top = bottom = left = right = *all;
            break;
        }
        case 2: {
            auto vertical = tokenizer.get();
            auto horizontal = tokenizer.next().get();
            assert(vertical.has_value() && horizontal.has_value());
            top = bottom = *vertical;
            left = right = *horizontal;
            break;
        }
        case 3: {
            auto top_edge = tokenizer.get();
            auto horizontal = tokenizer.next().get();
            auto bottom_edge = tokenizer.next().get();
            assert(top_edge.has_value() && horizontal.has_value() && bottom_edge.has_value());
            top = *top_edge;
            left = right = *horizontal;
            bottom = *bottom_edge;
            break;
        }
        case 4: {
            auto top_edge = tokenizer.get();
            auto right_edge = tokenizer.next().get();
            auto bottom_edge = tokenizer.next().get();
            auto left_edge = tokenizer.next().get();
            assert(top_edge.has_value() && right_edge.has_value() && bottom_edge.has_value() && left_edge.has_value());
            top = *top_edge;
            right = *right_edge;
            bottom = *bottom_edge;
            left = *left_edge;
            break;
        }
        default:
            break;
    }
    std::string_view post_fix;
    // The border properties aren't as simple as the padding or margin ones.
    if (property == "border-style") {
        property = "border";
        post_fix = "-style";
    } else if (property == "border-width") {
        property = "border";
        post_fix = "-width";
    } else if (property == "border-color") {
        property = "border";
        post_fix = "-color";
    }

    declarations.insert_or_assign(property_id_from_string(std::format("{}-top{}", property, post_fix)), top);
    declarations.insert_or_assign(property_id_from_string(std::format("{}-bottom{}", property, post_fix)), bottom);
    declarations.insert_or_assign(property_id_from_string(std::format("{}-left{}", property, post_fix)), left);
    declarations.insert_or_assign(property_id_from_string(std::format("{}-right{}", property, post_fix)), right);
}

void Parser::expand_font(Declarations &declarations, std::string_view value) {
    Tokenizer tokenizer(value, ' ');
    if (tokenizer.size() == 1) {
        // TODO(mkiael): Handle system properties correctly. Just forward it for now.
        auto system_property = tokenizer.get();
        assert(system_property.has_value());
        declarations.insert_or_assign(PropertyId::FontFamily, *system_property);
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
        }

        if (auto maybe_font_style = try_parse_font_style(tokenizer)) {
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

    if (font_size.empty() || font_family.empty()) {
        spdlog::warn("Unable to parse font size: '{}'", value);
        return;
    }

    declarations.insert_or_assign(PropertyId::FontStyle, font_style);
    declarations.insert_or_assign(PropertyId::FontVariant, font_variant);
    declarations.insert_or_assign(PropertyId::FontWeight, font_weight);
    declarations.insert_or_assign(PropertyId::FontStretch, font_stretch);
    declarations.insert_or_assign(PropertyId::FontSize, font_size);
    declarations.insert_or_assign(PropertyId::LineHeight, line_height);
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
