// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "style/styled_node.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>
#include <map>
#include <optional>
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
    auto it = std::ranges::find_if(properties, [=](auto const &p) { return p.first == property; });

    if (it == cend(properties)) {
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
    } else if (it->second == "unset") {
        // https://developer.mozilla.org/en-US/docs/Web/CSS/unset
        if (is_inherited(property) && parent != nullptr) {
            return parent->get_raw_property(property);
        }

        return kInitialValues.at(property);
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

DisplayValue StyledNode::get_display_property(css::PropertyId id) const {
    auto raw = get_raw_property(id);
    if (raw == "none") {
        return DisplayValue::None;
    } else if (raw == "inline") {
        return DisplayValue::Inline;
    } else if (raw == "block") {
        return DisplayValue::Block;
    }

    spdlog::warn("Unhandled display value {} for property {}", raw, static_cast<int>(id));
    return DisplayValue::None;
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
            auto it = std::ranges::find_if(
                    n->properties, [](auto const &v) { return v.first == css::PropertyId::FontSize; });
            if (it != end(n->properties)) {
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
    return static_cast<int>(value);
}

} // namespace style
