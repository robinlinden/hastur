// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "style/styled_node.h"

#include "gfx/color.h"
#include "util/string.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <map>
#include <optional>
#include <sstream>
#include <string_view>
#include <utility>

#include "util/from_chars.h"

using namespace std::literals;

namespace style {
namespace {

// https://www.w3.org/TR/css-cascade/#initial-values
// NOLINTNEXTLINE(cert-err58-cpp)
std::map<css::PropertyId, std::string_view> const initial_values{
        // https://developer.mozilla.org/en-US/docs/Web/CSS/background-color#formal_definition
        {css::PropertyId::BackgroundColor, "transparent"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/color#formal_definition
        {css::PropertyId::Color, "canvastext"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/font-size#formal_definition
        {css::PropertyId::FontSize, "medium"sv},
        // https://developer.mozilla.org/en-US/docs/Web/CSS/font-family#formal_definition
        {css::PropertyId::FontFamily, "arial"sv}, // TODO(robinlinden): Better default.
        // https://developer.mozilla.org/en-US/docs/Web/CSS/font-style#formal_definition
        {css::PropertyId::FontStyle, "normal"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/text-decoration
        {css::PropertyId::TextDecorationColor, "currentcolor"sv},
        {css::PropertyId::TextDecorationLine, "none"sv},
        {css::PropertyId::TextDecorationStyle, "solid"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/border-color#formal_definition
        {css::PropertyId::BorderBottomColor, "currentcolor"sv},
        {css::PropertyId::BorderLeftColor, "currentcolor"sv},
        {css::PropertyId::BorderRightColor, "currentcolor"sv},
        {css::PropertyId::BorderTopColor, "currentcolor"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/border-radius
        {css::PropertyId::BorderBottomLeftRadius, "0"sv},
        {css::PropertyId::BorderBottomRightRadius, "0"sv},
        {css::PropertyId::BorderTopLeftRadius, "0"sv},
        {css::PropertyId::BorderTopRightRadius, "0"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/border-style#formal_definition
        {css::PropertyId::BorderBottomStyle, "none"sv},
        {css::PropertyId::BorderLeftStyle, "none"sv},
        {css::PropertyId::BorderRightStyle, "none"sv},
        {css::PropertyId::BorderTopStyle, "none"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/border-width#formal_definition
        {css::PropertyId::BorderBottomWidth, "medium"sv},
        {css::PropertyId::BorderLeftWidth, "medium"sv},
        {css::PropertyId::BorderRightWidth, "medium"sv},
        {css::PropertyId::BorderTopWidth, "medium"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/padding#formal_definition
        {css::PropertyId::PaddingBottom, "0"sv},
        {css::PropertyId::PaddingLeft, "0"sv},
        {css::PropertyId::PaddingRight, "0"sv},
        {css::PropertyId::PaddingTop, "0"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/display#formal_definition
        {css::PropertyId::Display, "inline"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/height#formal_definition
        // https://developer.mozilla.org/en-US/docs/Web/CSS/max-height#formal_definition
        // https://developer.mozilla.org/en-US/docs/Web/CSS/min-height#formal_definition
        {css::PropertyId::Height, "auto"sv},
        {css::PropertyId::MaxHeight, "none"sv},
        {css::PropertyId::MinHeight, "auto"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/margin#formal_definition
        {css::PropertyId::MarginBottom, "0"sv},
        {css::PropertyId::MarginLeft, "0"sv},
        {css::PropertyId::MarginRight, "0"sv},
        {css::PropertyId::MarginTop, "0"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/width#formal_definition
        // https://developer.mozilla.org/en-US/docs/Web/CSS/max-width#formal_definition
        // https://developer.mozilla.org/en-US/docs/Web/CSS/min-width#formal_definition
        {css::PropertyId::Width, "auto"sv},
        {css::PropertyId::MaxWidth, "none"sv},
        {css::PropertyId::MinWidth, "auto"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/white-space
        {css::PropertyId::WhiteSpace, "normal"sv},
};

std::optional<gfx::Color> try_from_hex_chars(std::string_view hex_chars) {
    if (!hex_chars.starts_with('#')) {
        return std::nullopt;
    }

    hex_chars.remove_prefix(1);
    std::uint32_t hex{};
    if (hex_chars.length() == 6) {
        std::from_chars(hex_chars.data(), hex_chars.data() + hex_chars.size(), hex, /*base*/ 16);
        return gfx::Color::from_rgb(hex);
    } else if (hex_chars.length() == 3) {
        std::ostringstream ss;
        ss << hex_chars[0] << hex_chars[0] << hex_chars[1] << hex_chars[1] << hex_chars[2] << hex_chars[2];
        auto expanded = std::move(ss).str();
        std::from_chars(expanded.data(), expanded.data() + expanded.size(), hex, /*base*/ 16);
        return gfx::Color::from_rgb(hex);
    } else if (hex_chars.length() == 8) {
        std::from_chars(hex_chars.data(), hex_chars.data() + hex_chars.size(), hex, /*base*/ 16);
        return gfx::Color::from_rgba(hex);
    } else if (hex_chars.length() == 4) {
        std::ostringstream ss;
        ss << hex_chars[0] << hex_chars[0] << hex_chars[1] << hex_chars[1] << hex_chars[2] << hex_chars[2]
           << hex_chars[3] << hex_chars[3];
        auto expanded = std::move(ss).str();
        std::from_chars(expanded.data(), expanded.data() + expanded.size(), hex, /*base*/ 16);
        return gfx::Color::from_rgba(hex);
    }

    return std::nullopt;
}

// https://developer.mozilla.org/en-US/docs/Web/CSS/color_value/rgb
std::optional<gfx::Color> try_from_rgba(std::string_view text) {
    if (text.starts_with("rgb(")) {
        text.remove_prefix(std::strlen("rgb("));
    } else if (text.starts_with("rgba(")) {
        text.remove_prefix(std::strlen("rgba("));
    } else {
        return std::nullopt;
    }

    if (!text.ends_with(')')) {
        return std::nullopt;
    }
    text.remove_suffix(std::strlen(")"));

    // First try to handle rgba(1, 2, 3, .5)
    auto rgba = util::split(text, ",");
    if (rgba.size() == 1) {
        // And then rgba(1 2 3 / .5)
        rgba = util::split(text, "/");
        if (rgba.size() == 2) {
            auto a = rgba[1];
            rgba = util::split(rgba[0], " ");
            rgba.push_back(a);
        } else {
            rgba = util::split(text, " ");
        }

        // Nuke any empty segments. This happens if you have more than 1 space
        // between the rgba arguments.
        std::erase_if(rgba, [](auto const &s) { return empty(s); });
    }

    if (rgba.size() != 3 && rgba.size() != 4) {
        return std::nullopt;
    }

    for (auto &value : rgba) {
        value = util::trim(value);
    }

    auto to_int = [](std::string_view v) {
        int ret{-1};
        if (std::from_chars(v.data(), v.data() + v.size(), ret).ptr != v.data() + v.size()) {
            return -1;
        }

        if (ret < 0 || ret > 255) {
            return -1;
        }

        return ret;
    };

    auto r{to_int(rgba[0])};
    auto g{to_int(rgba[1])};
    auto b{to_int(rgba[2])};
    if (r == -1 || g == -1 || b == -1) {
        return std::nullopt;
    }

    if (rgba.size() == 3) {
        return gfx::Color{static_cast<std::uint8_t>(r), static_cast<std::uint8_t>(g), static_cast<std::uint8_t>(b)};
    }

    float a{-1.f};
    if (util::from_chars(rgba[3].data(), rgba[3].data() + rgba[3].size(), a).ptr != rgba[3].data() + rgba[3].size()) {
        return std::nullopt;
    }

    a = std::clamp(a, 0.f, 1.f);

    return gfx::Color{
            static_cast<std::uint8_t>(r),
            static_cast<std::uint8_t>(g),
            static_cast<std::uint8_t>(b),
            static_cast<std::uint8_t>(a * 255),
    };
}

gfx::Color parse_color(std::string_view str) {
    if (auto color = try_from_hex_chars(str)) {
        return *color;
    }

    if (auto color = try_from_rgba(str)) {
        return *color;
    }

    if (auto css_named_color = gfx::Color::from_css_name(str)) {
        return *css_named_color;
    }

    spdlog::warn("Unrecognized color format: {}", str);
    return gfx::Color{0xFF, 0, 0};
}

std::string_view get_parent_raw_property(style::StyledNode const &node, css::PropertyId property) {
    if (node.parent != nullptr) {
        return node.parent->get_raw_property(property);
    }

    return initial_values.at(property);
}

std::optional<std::pair<float, std::string_view>> split_into_value_and_unit(std::string_view property) {
    float res{};
    auto parse_result = util::from_chars(property.data(), property.data() + property.size(), res);
    if (parse_result.ec != std::errc{}) {
        spdlog::warn("Unable to split '{}' in split_into_value_and_unit", property);
        return std::nullopt;
    }

    auto const parsed_length = std::distance(property.data(), parse_result.ptr);
    auto const unit = property.substr(parsed_length);
    return std::pair{res, unit};
}

} // namespace

std::string_view StyledNode::get_raw_property(css::PropertyId property) const {
    // We don't support selector specificity yet, so the last property is found
    // in order to allow website style to override the browser built-in style.
    auto it = std::ranges::find_if(
            rbegin(properties), rend(properties), [=](auto const &p) { return p.first == property; });

    // TODO(robinlinden): Having a special case for dom::Text here doesn't feel good.
    // You can't set properties on text nodes in HTML (even though we do in
    // tests), so let's grab this from the parent node.
    if (it == rend(properties) && std::holds_alternative<dom::Text>(node) && parent != nullptr) {
        return parent->get_raw_property(property);
    }

    if (it == rend(properties) || it->second == "unset") {
        // https://developer.mozilla.org/en-US/docs/Web/CSS/unset
        if (is_inherited(property) && parent != nullptr) {
            return parent->get_raw_property(property);
        }

        return initial_values.at(property);
    } else if (it->second == "initial") {
        // https://developer.mozilla.org/en-US/docs/Web/CSS/initial
        return initial_values.at(property);
    } else if (it->second == "inherit") {
        // https://developer.mozilla.org/en-US/docs/Web/CSS/inherit
        return get_parent_raw_property(*this, property);
    } else if (it->second == "currentcolor") {
        // https://developer.mozilla.org/en-US/docs/Web/CSS/color_value#currentcolor_keyword
        // If the "color" property has the value "currentcolor", treat it as "inherit".
        if (it->first == css::PropertyId::Color) {
            return get_parent_raw_property(*this, property);
        }

        // Even though we return the correct value here, if a property has
        // "currentcolor" as its initial value, the caller have to manually look
        // up the value of "color". This will be cleaned up along with the rest
        // of the property management soon.
        return get_raw_property(css::PropertyId::Color);
    }

    return it->second;
}

BorderStyle StyledNode::get_border_style_property(css::PropertyId property) const {
    auto raw = get_raw_property(property);

    if (raw == "none") {
        return BorderStyle::None;
    } else if (raw == "hidden") {
        return BorderStyle::Hidden;
    } else if (raw == "dotted") {
        return BorderStyle::Dotted;
    } else if (raw == "dashed") {
        return BorderStyle::Dashed;
    } else if (raw == "solid") {
        return BorderStyle::Solid;
    } else if (raw == "double") {
        return BorderStyle::Double;
    } else if (raw == "groove") {
        return BorderStyle::Groove;
    } else if (raw == "ridge") {
        return BorderStyle::Ridge;
    } else if (raw == "inset") {
        return BorderStyle::Inset;
    } else if (raw == "outset") {
        return BorderStyle::Outset;
    }

    spdlog::warn("Unhandled border-style value '{}'", raw);
    return BorderStyle::None;
}

gfx::Color StyledNode::get_color_property(css::PropertyId property) const {
    auto color_text = get_raw_property(property);

    // https://developer.mozilla.org/en-US/docs/Web/CSS/color_value#currentcolor_keyword
    if (color_text == "currentcolor") {
        color_text = get_raw_property(css::PropertyId::Color);
    }

    return parse_color(color_text);
}

DisplayValue StyledNode::get_display_property() const {
    auto raw = get_raw_property(css::PropertyId::Display);
    if (raw == "none") {
        return DisplayValue::None;
    } else if (raw == "inline") {
        return DisplayValue::Inline;
    } else if (raw == "block") {
        return DisplayValue::Block;
    }

    spdlog::warn("Unhandled display value '{}'", raw);
    return DisplayValue::Block;
}

FontStyle StyledNode::get_font_style_property() const {
    auto raw = get_raw_property(css::PropertyId::FontStyle);
    if (raw == "normal") {
        return FontStyle::Normal;
    } else if (raw == "italic") {
        return FontStyle::Italic;
    } else if (raw == "oblique") {
        return FontStyle::Oblique;
    }

    spdlog::warn("Unhandled font style value {}", raw);
    return FontStyle::Normal;
}

std::vector<TextDecorationLine> StyledNode::get_text_decoration_line_property() const {
    auto into = [](std::string_view v) -> std::optional<TextDecorationLine> {
        if (v == "none") {
            return TextDecorationLine::None;
        } else if (v == "underline") {
            return TextDecorationLine::Underline;
        } else if (v == "overline") {
            return TextDecorationLine::Overline;
        } else if (v == "line-through") {
            return TextDecorationLine::LineThrough;
        } else if (v == "blink") {
            return TextDecorationLine::Blink;
        }

        spdlog::warn("Unhandled text-decoration-line value '{}'", v);
        return std::nullopt;
    };

    std::vector<TextDecorationLine> lines;

    auto parts = util::split(get_raw_property(css::PropertyId::TextDecorationLine), " ");
    for (auto const &part : parts) {
        if (auto line = into(part)) {
            lines.push_back(*line);
        } else {
            return {};
        }
    }

    return lines;
}

static constexpr int kDefaultFontSize{16};
// https://drafts.csswg.org/css-fonts-4/#absolute-size-mapping
constexpr int kMediumFontSize = kDefaultFontSize;
// NOLINTNEXTLINE(cert-err58-cpp)
std::map<std::string_view, float> const font_size_absolute_size_keywords{
        {"xx-small", 3 / 5.f},
        {"x-small", 3 / 4.f},
        {"small", 8 / 9.f},
        {"medium", 1.f},
        {"large", 6 / 5.f},
        {"x-large", 3 / 2.f},
        {"xx-large", 2 / 1.f},
        {"xxx-large", 3 / 1.f},
};

int StyledNode::get_font_size_property() const {
    auto get_closest_font_size_and_owner =
            [](StyledNode const *starting_node) -> std::optional<std::pair<std::string_view, StyledNode const *>> {
        for (auto const *n = starting_node; n != nullptr; n = n->parent) {
            auto it = std::ranges::find_if(rbegin(n->properties), rend(n->properties), [](auto const &v) {
                return v.first == css::PropertyId::FontSize;
            });
            if (it != rend(n->properties)) {
                return {{it->second, n}};
            }
        }

        return std::nullopt;
    };

    auto closest = get_closest_font_size_and_owner(this);
    if (!closest) {
        return kDefaultFontSize;
    }
    auto raw_value = closest->first;

    if (font_size_absolute_size_keywords.contains(raw_value)) {
        return std::lround(font_size_absolute_size_keywords.at(raw_value) * kMediumFontSize);
    }

    auto parent_or_default_font_size = [&] {
        auto const *owner = closest->second;
        if (owner->parent == nullptr) {
            return kDefaultFontSize;
        }

        return owner->parent->get_font_size_property();
    };

    // https://drafts.csswg.org/css-fonts-4/#valdef-font-size-relative-size
    constexpr auto kRelativeFontSizeRatio = 1.2f;
    if (raw_value == "larger") {
        return static_cast<int>(parent_or_default_font_size() * kRelativeFontSizeRatio);
    }

    if (raw_value == "smaller") {
        return static_cast<int>(parent_or_default_font_size() / kRelativeFontSizeRatio);
    }

    auto value_and_unit = split_into_value_and_unit(raw_value);
    if (!value_and_unit) {
        return kDefaultFontSize;
    }
    auto [value, unit] = *value_and_unit;

    if (value == 0) {
        return 0;
    }

    if (unit == "px") {
        return static_cast<int>(value);
    }

    if (unit == "em") {
        return static_cast<int>(value * parent_or_default_font_size());
    }

    if (unit == "%") {
        return static_cast<int>(value / 100.f * parent_or_default_font_size());
    }

    if (unit == "rem") {
        auto const *root = [&] {
            auto const *n = closest->second;
            while (n->parent) {
                n = n->parent;
            }
            return n;
        }();
        auto root_font_size = root && root != this ? root->get_font_size_property() : kDefaultFontSize;
        return static_cast<int>(value * root_font_size);
    }

    if (unit == "pt") {
        // 12pt seems to generally equal 16px.
        static constexpr float kPtToPxRatio = 16.f / 12.f;
        return static_cast<int>(value * kPtToPxRatio);
    }

    spdlog::warn("Unhandled unit '{}'", unit);
    return 0;
}

std::optional<WhiteSpace> StyledNode::get_white_space_property() const {
    auto raw = get_raw_property(css::PropertyId::WhiteSpace);
    if (raw == "normal") {
        return WhiteSpace::Normal;
    }

    if (raw == "pre") {
        return WhiteSpace::Pre;
    }

    if (raw == "nowrap") {
        return WhiteSpace::Nowrap;
    }

    if (raw == "pre-wrap") {
        return WhiteSpace::PreWrap;
    }

    if (raw == "break-spaces") {
        return WhiteSpace::BreakSpaces;
    }

    if (raw == "pre-line") {
        return WhiteSpace::PreLine;
    }

    spdlog::warn("Unhandled white-space '{}'", raw);
    return std::nullopt;
}

} // namespace style
