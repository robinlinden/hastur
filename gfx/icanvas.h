// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef GFX_ICANVAS_H_
#define GFX_ICANVAS_H_

#include "geom/geom.h"
#include "gfx/color.h"
#include "gfx/font.h"

#include <string_view>

namespace gfx {

struct BorderProperties {
    Color color{};
    int size{};
    [[nodiscard]] bool operator==(BorderProperties const &) const = default;
};

struct Borders {
    BorderProperties left{}, right{}, top{}, bottom{};
    [[nodiscard]] bool operator==(Borders const &) const = default;
};

class ICanvas {
public:
    virtual ~ICanvas() = default;

    virtual void set_viewport_size(int width, int height) = 0;
    virtual void set_scale(int scale) = 0;
    virtual void add_translation(int dx, int dy) = 0;
    virtual void fill_rect(geom::Rect const &, Color) = 0;
    virtual void draw_border(geom::Rect const &, Borders const &) = 0;
    virtual void draw_text(geom::Position, std::string_view, Font, FontSize, Color) = 0;
};

} // namespace gfx

#endif
