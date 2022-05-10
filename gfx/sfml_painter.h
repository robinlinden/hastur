// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef GFX_SFML_PAINTER_H_
#define GFX_SFML_PAINTER_H_

#include "gfx/ipainter.h"

namespace sf {
class RenderTarget;
} // namespace sf

namespace gfx {

class SfmlPainter : public IPainter {
public:
    SfmlPainter(sf::RenderTarget &target) : target_{target} {}

    void set_viewport_size(int width, int height) override;
    constexpr void set_scale(int scale) override { scale_ = scale; }

    constexpr void add_translation(int dx, int dy) override {
        tx_ += dx;
        ty_ += dy;
    }

    void fill_rect(geom::Rect const &, Color) override;
    void draw_text(geom::Position, std::string_view, Font, FontSize, Color) override {}

private:
    sf::RenderTarget &target_;

    int scale_{1};
    int tx_{0};
    int ty_{0};
};

} // namespace gfx

#endif
