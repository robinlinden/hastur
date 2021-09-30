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
    bool operator==(EdgeSize const &) const = default;
};

struct Rect {
    int x{}, y{}, width{}, height{};
    bool operator==(Rect const &) const = default;
};

} // namespace geom

#endif
