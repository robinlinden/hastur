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
    int x{}, y{};
};

struct Rect {
    int x{}, y{}, width{}, height{};
    bool operator==(Rect const &) const = default;
};

struct EdgeSize {
    int left{}, right{}, top{}, bottom{};
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

    bool contains(Position p) const {
        auto bounds = border_box();
        bool inside_horizontal = p.x >= bounds.x && p.x <= bounds.x + bounds.width;
        bool inside_vertical = p.y >= bounds.y && p.y <= bounds.y + bounds.height;
        return inside_horizontal && inside_vertical;
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
