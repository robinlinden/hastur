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
std::map<css::PropertyId, std::string_view> const kInitialValues{
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

        // https://developer.mozilla.org/en-US/docs/Web/CSS/border-color#formal_definition
        {css::PropertyId::BorderBottomColor, "currentcolor"sv},
        {css::PropertyId::BorderLeftColor, "currentcolor"sv},
        {css::PropertyId::BorderRightColor, "currentcolor"sv},
        {css::PropertyId::BorderTopColor, "currentcolor"sv},

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

// TODO(robinlinden): space-separated values.
// https://developer.mozilla.org/en-US/docs/Web/CSS/color_value/rgb
// https://developer.mozilla.org/en-US/docs/Web/CSS/color_value/rgba
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

    auto rgba = util::split(text, ",");
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

    if (a < 0.f || a > 1.f) {
        return std::nullopt;
    }

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

    return kInitialValues.at(property);
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

    if (it == rend(properties) || it->second == "unset") {
        // https://developer.mozilla.org/en-US/docs/Web/CSS/unset
        if (is_inherited(property) && parent != nullptr) {
            return parent->get_raw_property(property);
        }

        return kInitialValues.at(property);
    } else if (it->second == "initial") {
        // https://developer.mozilla.org/en-US/docs/Web/CSS/initial
        return kInitialValues.at(property);
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

static int const kDefaultFontSize{10};
// https://w3c.github.io/csswg-drafts/css-fonts-4/#absolute-size-mapping
constexpr int kMediumFontSize = kDefaultFontSize;
std::map<std::string_view, float> const kFontSizeAbsoluteSizeKeywords{
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

    if (kFontSizeAbsoluteSizeKeywords.contains(raw_value)) {
        return std::lround(kFontSizeAbsoluteSizeKeywords.at(raw_value) * kMediumFontSize);
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

    auto parent_or_default_font_size = [&] {
        auto const *owner = closest->second;
        if (owner->parent == nullptr) {
            return kDefaultFontSize;
        }

        return owner->parent->get_font_size_property();
    };

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

    spdlog::warn("Unhandled unit '{}'", unit);
    return 0;
}

} // namespace style
