// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "style/styled_node.h"

#include <algorithm>
#include <array>
#include <string_view>

using namespace std::literals;

namespace style {
namespace {
// https://www.w3.org/TR/CSS22/propidx.html
constexpr std::array kInheritedProperties{
        "azimuth"sv,
        "border-collapse"sv,
        "border-spacing"sv,
        "caption-side"sv,
        "color"sv,
        "cursor"sv,
        "direction"sv,
        "elevation"sv,
        "empty-cells"sv,
        "font"sv,
        "font-family"sv,
        "font-size"sv,
        "font-style"sv,
        "font-variant"sv,
        "font-weight"sv,
        "letter-spacing"sv,
        "line-height"sv,
        "list-style"sv,
        "list-style-image"sv,
        "list-style-position"sv,
        "list-style-type"sv,
        "orphans"sv,
        "pitch"sv,
        "pitch-range"sv,
        "quotes"sv,
        "richness"sv,
        "speak"sv,
        "speak-header"sv,
        "speak-numeral"sv,
        "speak-punctuation"sv,
        "speech-rate"sv,
        "stress"sv,
        "text-align"sv,
        "text-indent"sv,
        "text-transform"sv,
        "visibility"sv,
        "voice-family"sv,
        "volume"sv,
        "widows"sv,
        "word-spacing"sv,
};

template<auto const &array>
constexpr bool is_in_array(std::string_view str) {
    return std::ranges::find(array, str) != std::cend(array);
}

constexpr bool is_inherited(std::string_view property) {
    return is_in_array<kInheritedProperties>(property);
}

// https://www.w3.org/TR/css-cascade/#initial-values
std::map<std::string_view, std::string_view> const kInitialValues{
        // https://developer.mozilla.org/en-US/docs/Web/CSS/background-color#formal_definition
        {"background-color"sv, "transparent"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/color#formal_definition
        {"color"sv, "canvastext"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/font-size#formal_definition
        {"font-size"sv, "medium"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/border-color#formal_definition
        {"border-bottom-color"sv, "currentcolor"sv},
        {"border-left-color"sv, "currentcolor"sv},
        {"border-right-color"sv, "currentcolor"sv},
        {"border-top-color"sv, "currentcolor"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/border-style#formal_definition
        {"border-bottom-style"sv, "none"sv},
        {"border-left-style"sv, "none"sv},
        {"border-right-style"sv, "none"sv},
        {"border-top-style"sv, "none"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/border-width#formal_definition
        {"border-bottom-width"sv, "medium"sv},
        {"border-left-width"sv, "medium"sv},
        {"border-right-width"sv, "medium"sv},
        {"border-top-width"sv, "medium"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/padding#formal_definition
        {"padding-bottom"sv, "0"sv},
        {"padding-left"sv, "0"sv},
        {"padding-right"sv, "0"sv},
        {"padding-top"sv, "0"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/display#formal_definition
        {"display"sv, "inline"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/height#formal_definition
        // https://developer.mozilla.org/en-US/docs/Web/CSS/max-height#formal_definition
        // https://developer.mozilla.org/en-US/docs/Web/CSS/min-height#formal_definition
        {"height"sv, "auto"sv},
        {"max-height"sv, "none"sv},
        {"min-height"sv, "auto"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/margin#formal_definition
        {"margin-bottom"sv, "0"sv},
        {"margin-left"sv, "0"sv},
        {"margin-right"sv, "0"sv},
        {"margin-top"sv, "0"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/width#formal_definition
        // https://developer.mozilla.org/en-US/docs/Web/CSS/max-width#formal_definition
        // https://developer.mozilla.org/en-US/docs/Web/CSS/min-width#formal_definition
        {"width"sv, "auto"sv},
        {"max-width"sv, "none"sv},
        {"min-width"sv, "auto"sv},
};

std::string_view get_parent_property(style::StyledNode const &node, std::string_view property) {
    if (node.parent != nullptr) {
        return node.parent->get_property(property);
    }

    return kInitialValues.at(property);
}

} // namespace

std::string_view StyledNode::get_property(std::string_view property) const {
    auto it = std::ranges::find_if(properties, [=](auto const &p) { return p.first == property; });

    if (it == cend(properties)) {
        if (is_inherited(property) && parent != nullptr) {
            return parent->get_property(property);
        }

        return kInitialValues.at(property);
    } else if (it->second == "initial") {
        // https://developer.mozilla.org/en-US/docs/Web/CSS/initial
        return kInitialValues.at(property);
    } else if (it->second == "inherit") {
        // https://developer.mozilla.org/en-US/docs/Web/CSS/inherit
        return get_parent_property(*this, property);
    } else if (it->second == "currentcolor") {
        // https://developer.mozilla.org/en-US/docs/Web/CSS/color_value#currentcolor_keyword
        // If the "color" property has the value "currentcolor", treat it as "inherit".
        if (it->first == "color") {
            return get_parent_property(*this, property);
        }

        // Even though we return the correct value here, if a property has
        // "currentcolor" as its initial value, the caller have to manually look
        // up the value of "color". This will be cleaned up along with the rest
        // of the property management soon.
        return get_property("color");
    }

    return it->second;
}

} // namespace style
