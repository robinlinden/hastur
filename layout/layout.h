// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef LAYOUT_LAYOUT_H_
#define LAYOUT_LAYOUT_H_

#include "style/styled_node.h"

#include <string>
#include <vector>

namespace layout {

struct Position {
    float x{}, y{};
};

struct Rect {
    float x{}, y{}, width{}, height{};
    bool operator==(Rect const &) const = default;
};

struct EdgeSize {
    float left{}, right{}, top{}, bottom{};
    bool operator==(EdgeSize const &) const = default;
};

struct BoxModel {
    Rect content{};

    EdgeSize padding{};
    EdgeSize border{};
    EdgeSize margin{};

    bool operator==(BoxModel const &) const = default;

    Rect padding_box() const;
    Rect border_box() const;
    Rect margin_box() const;

    constexpr bool contains(Position p) const {
        bool right_of_left_edge = p.x >= content.x - padding.left - border.left;
        bool left_of_right_edge = p.x <= content.x + content.width + padding.right + border.right;
        bool below_top = p.y >= content.y - padding.top - border.top;
        bool above_bottom = p.y <= content.y + content.height + padding.bottom + border.bottom;
        return right_of_left_edge && left_of_right_edge && below_top && above_bottom;
    }
};

enum class LayoutType {
    Inline,
    Block,
    AnonymousBlock, // Holds groups of sequential inline boxes.
};

struct LayoutBox {
    style::StyledNode const *node;
    LayoutType type;
    BoxModel dimensions;
    std::vector<LayoutBox> children;
    bool operator==(LayoutBox const &) const = default;
};

LayoutBox create_layout(style::StyledNode const &node, int width);

LayoutBox const *box_at_position(LayoutBox const &, Position);

std::string to_string(LayoutBox const &box);

} // namespace layout

#endif
