// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef GEOM_GEOM_H_
#define GEOM_GEOM_H_

#include <algorithm>

namespace geom {

struct Position {
    int x{}, y{};
    [[nodiscard]] bool operator==(Position const &) const = default;

    [[nodiscard]] constexpr Position scaled(unsigned scale, Position origin = {0, 0}) const {
        return Position{
                origin.x + (x - origin.x) * static_cast<int>(scale),
                origin.y + (y - origin.y) * static_cast<int>(scale),
        };
    }

    [[nodiscard]] constexpr Position translated(int dx, int dy) const { return {x + dx, y + dy}; }
};

struct EdgeSize {
    int left{}, right{}, top{}, bottom{};
    [[nodiscard]] bool operator==(EdgeSize const &) const = default;
};

struct Rect {
    int x{}, y{}, width{}, height{};
    [[nodiscard]] bool operator==(Rect const &) const = default;

    [[nodiscard]] constexpr int left() const { return x; }
    [[nodiscard]] constexpr int right() const { return x + width; }
    [[nodiscard]] constexpr int top() const { return y; }
    [[nodiscard]] constexpr int bottom() const { return y + height; }

    [[nodiscard]] constexpr Position position() const { return {x, y}; }

    [[nodiscard]] constexpr Rect expanded(EdgeSize const &edges) const {
        return Rect{
                left() - edges.left,
                top() - edges.top,
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
        auto new_left = std::max(left(), other.left());
        auto new_right = std::min(right(), other.right());
        auto new_top = std::max(top(), other.top());
        auto new_bottom = std::min(bottom(), other.bottom());
        if (new_left > new_right || new_top > new_bottom) {
            return {};
        }

        return Rect{
                new_left,
                new_top,
                new_right - new_left,
                new_bottom - new_top,
        };
    }

    [[nodiscard]] constexpr bool contains(Position const &p) const {
        bool inside_horizontally = p.x >= left() && p.x <= right();
        bool inside_vertically = p.y >= top() && p.y <= bottom();
        return inside_vertically && inside_horizontally;
    }
};

} // namespace geom

#endif
