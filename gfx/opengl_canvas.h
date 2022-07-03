// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef GFX_OPENGL_CANVAS_H_
#define GFX_OPENGL_CANVAS_H_

#include "gfx/icanvas.h"

namespace gfx {

class OpenGLCanvas final : public ICanvas {
public:
    OpenGLCanvas();

    void set_viewport_size(int width, int height) override;
    constexpr void set_scale(int scale) override { scale_ = scale; }

    constexpr void add_translation(int dx, int dy) override {
        translation_x += dx;
        translation_y += dy;
    }

    void fill_rect(geom::Rect const &, Color) override;
    void draw_rect(geom::Rect const &, Color const &, Borders const &) override {}
    void draw_text(geom::Position, std::string_view, Font, FontSize, Color) override {}

private:
    int translation_x{};
    int translation_y{};
    int scale_{1};
};

} // namespace gfx

#endif
