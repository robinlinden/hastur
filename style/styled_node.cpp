// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "style/styled_node.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <string_view>

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

} // namespace style
