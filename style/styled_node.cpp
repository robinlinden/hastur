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
        {"width"sv, "auto"sv}, // https://drafts.csswg.org/css-sizing-3/#propdef-width
};

std::optional<std::string_view> get_parent_property(style::StyledNode const &node, std::string_view property) {
    if (node.parent != nullptr) {
        return get_property(*node.parent, property);
    }

    if (kInitialValues.contains(property)) {
        return kInitialValues.at(property);
    }

    return std::nullopt;
}

} // namespace

std::optional<std::string_view> get_property(style::StyledNode const &node, std::string_view property) {
    auto it = std::ranges::find_if(node.properties, [=](auto const &p) { return p.first == property; });

    if (it == cend(node.properties)) {
        if (is_inherited(property) && node.parent != nullptr) {
            return get_property(*node.parent, property);
        }

        if (kInitialValues.contains(property)) {
            return kInitialValues.at(property);
        }

        return std::nullopt;
    } else if (it->second == "inherit") {
        // https://developer.mozilla.org/en-US/docs/Web/CSS/inherit
        return get_parent_property(node, property);
    } else if (it->second == "currentcolor") {
        // https://developer.mozilla.org/en-US/docs/Web/CSS/color_value#currentcolor_keyword
        // If the "color" property has the value "currentcolor", treat it as "inherit".
        if (it->first == "color") {
            return get_parent_property(node, property);
        }

        // Even though we return the correct value here, if a property has
        // "currentcolor" as its initial value, the caller have to manually look
        // up the value of "color". This will be cleaned up along with the rest
        // of the property management soon.
        return get_property(node, "color");
    }

    return it->second;
}

} // namespace style
