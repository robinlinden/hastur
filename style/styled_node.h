// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef STYLE_STYLED_NODE_H_
#define STYLE_STYLED_NODE_H_

#include "dom/dom.h"

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace style {

// Using reference_wrapper here because I want this to be movable and copy-constructible.
struct StyledNode {
    std::reference_wrapper<dom::Node const> node;
    std::vector<std::pair<std::string, std::string>> properties;
    std::vector<StyledNode> children;
};

[[nodiscard]] inline bool operator==(style::StyledNode const &a, style::StyledNode const &b) noexcept {
    return a.node.get() == b.node.get() && a.properties == b.properties && a.children == b.children;
}

std::optional<std::string_view> get_property(StyledNode const &node, std::string_view property);
std::string_view get_property_or(StyledNode const &node, std::string_view property, std::string_view fallback);

} // namespace style

#endif
