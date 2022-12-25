// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef GFX_PAINTER_H_
#define GFX_PAINTER_H_

#include "gfx/icanvas.h"

namespace gfx {

class Painter {
public:
    Painter(ICanvas &canvas) : canvas_{canvas} {}

    void fill_rect(geom::Rect const &rect, Color color) { canvas_.fill_rect(rect, color); }

    void draw_rect(geom::Rect const &rect, Color const &color, Borders const &borders) {
        canvas_.draw_rect(rect, color, borders);
    }

    void draw_text(geom::Position p, std::string_view text, Font font, FontSize size, FontStyle style, Color color) {
        canvas_.draw_text(p, text, font, size, style, color);
    }

private:
    ICanvas &canvas_;
};

} // namespace gfx

#endif
