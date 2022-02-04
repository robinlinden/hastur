// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef GEOM_GEOM_H_
#define GEOM_GEOM_H_

#include <algorithm>

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

    [[nodiscard]] constexpr Rect scaled(unsigned scale, Position origin = {0, 0}) const {
        return Rect{
                origin.x + (x - origin.x) * static_cast<int>(scale),
                origin.y + (y - origin.y) * static_cast<int>(scale),
                static_cast<int>(width * scale),
                static_cast<int>(height * scale),
        };
    }

    [[nodiscard]] constexpr Rect translated(int dx, int dy) const { return {x + dx, y + dy, width, height}; }

    [[nodiscard]] constexpr Rect intersected(Rect const &other) const {
        auto left = std::max(x, other.x);
        auto right = std::min(x + width, other.x + other.width);
        auto top = std::max(y, other.y);
        auto bottom = std::min(y + height, other.y + other.height);
        if (left > right || top > bottom) {
            return {};
        }

        return Rect{
                left,
                top,
                right - left,
                bottom - top,
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
