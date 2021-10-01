// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef GEOM_GEOM_H_
#define GEOM_GEOM_H_

namespace geom {

struct Position {
    int x{}, y{};
};

struct EdgeSize {
    int left{}, right{}, top{}, bottom{};
    [[nodiscard]] bool operator==(EdgeSize const &) const = default;
};

struct Rect {
    int x{}, y{}, width{}, height{};
    [[nodiscard]] bool operator==(Rect const &) const = default;

    [[nodiscard]] constexpr Rect expanded(EdgeSize const &edges) const {
        return Rect{
                x - edges.left,
                y - edges.top,
                edges.left + width + edges.right,
                edges.top + height + edges.bottom,
        };
    }

    [[nodiscard]] constexpr bool contains(Position const &p) const {
        bool inside_horizontally = p.x >= x && p.x <= x + width;
        bool inside_vertically = p.y >= y && p.y <= y + height;
        return inside_vertically && inside_horizontally;
    }
};

} // namespace geom

#endif
