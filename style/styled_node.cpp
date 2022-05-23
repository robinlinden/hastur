// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "style/styled_node.h"

#include <algorithm>

namespace style {

std::optional<std::string_view> get_property(style::StyledNode const &node, std::string_view property) {
    auto it = std::find_if(
            cbegin(node.properties), cend(node.properties), [=](auto const &p) { return p.first == property; });

    if (it == cend(node.properties)) {
        return std::nullopt;
    }

    return it->second;
}

std::string_view get_property_or(style::StyledNode const &node, std::string_view property, std::string_view fallback) {
    if (auto prop = get_property(node, property)) {
        return *prop;
    }
    return fallback;
}

} // namespace style
