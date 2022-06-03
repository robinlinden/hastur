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
    return std::find(std::cbegin(array), std::cend(array), str) != std::cend(array);
}

constexpr bool is_inherited(std::string_view property) {
    return is_in_array<kInheritedProperties>(property);
}
} // namespace

std::optional<std::string_view> get_property(style::StyledNode const &node, std::string_view property) {
    auto it = std::find_if(
            cbegin(node.properties), cend(node.properties), [=](auto const &p) { return p.first == property; });

    if (it == cend(node.properties)) {
        if (is_inherited(property) && node.parent != nullptr) {
            return get_property(*node.parent, property);
        }

        return std::nullopt;
    }

    return it->second;
}

} // namespace style
