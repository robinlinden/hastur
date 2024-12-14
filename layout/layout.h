// SPDX-FileCopyrightText: 2021-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef LAYOUT_LAYOUT_H_
#define LAYOUT_LAYOUT_H_

#include "layout/layout_box.h"

#include "style/styled_node.h"
#include "type/naive.h"
#include "type/type.h"

#include <functional>
#include <optional>
#include <string_view>

namespace layout {

struct LayoutInfo {
    int viewport_width{};
    int viewport_height{};
};

struct Size {
    int width{};
    int height{};
    [[nodiscard]] bool operator==(Size const &) const = default;
};

std::optional<LayoutBox> create_layout(
        style::StyledNode const &,
        LayoutInfo const &,
        type::IType const & = type::NaiveType{},
        std::function<std::optional<Size>(std::string_view)> const &get_intrensic_size_for_resource_at_url =
                [](std::string_view) { return std::nullopt; });

inline std::optional<LayoutBox> create_layout(
        style::StyledNode const &node,
        int width,
        type::IType const &type = type::NaiveType{},
        std::function<std::optional<Size>(std::string_view)> const &get_intrensic_size_for_resource_at_url =
                [](std::string_view) { return std::nullopt; }) {
    return create_layout(node, {width, 0}, type, get_intrensic_size_for_resource_at_url);
}

} // namespace layout

#endif
