// SPDX-FileCopyrightText: 2021-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef GFX_ICANVAS_H_
#define GFX_ICANVAS_H_

#include "geom/geom.h"
#include "gfx/color.h"
#include "gfx/font.h"

#include <cstdint>
#include <span>
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

struct Radii {
    int horizontal{};
    int vertical{};
    [[nodiscard]] bool operator==(Radii const &) const = default;
};

struct Corners {
    Radii top_left{};
    Radii top_right{};
    Radii bottom_left{};
    Radii bottom_right{};
    [[nodiscard]] bool operator==(Corners const &) const = default;
};

class ICanvas {
public:
    virtual ~ICanvas() = default;

    virtual void set_viewport_size(int width, int height) = 0;
    virtual void set_scale(int scale) = 0;
    virtual void add_translation(int dx, int dy) = 0;
    virtual void clear(Color) = 0;
    virtual void draw_rect(geom::Rect const &, Color const &, Borders const &, Corners const &) = 0;
    virtual void draw_text(geom::Position, std::string_view, std::span<Font const>, FontSize, FontStyle, Color) = 0;
    virtual void draw_text(geom::Position, std::string_view, Font, FontSize, FontStyle, Color) = 0;
    virtual void draw_pixels(geom::Rect const &, std::span<std::uint8_t const> rgba_data) = 0;
};

} // namespace gfx

#endif
