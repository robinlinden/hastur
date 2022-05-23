// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "style/styled_node.h"

#include <algorithm>
#include <string_view>

using namespace std::literals;

namespace style {
namespace {
constexpr bool is_inherited(std::string_view property) {
    return property == "font-size"sv;
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
