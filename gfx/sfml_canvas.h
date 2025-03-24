// SPDX-FileCopyrightText: 2022-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef GFX_SFML_CANVAS_H_
#define GFX_SFML_CANVAS_H_

#include "gfx/icanvas.h"

#include "geom/geom.h"
#include "gfx/color.h"
#include "gfx/font.h"
#include "type/sfml.h"

#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Texture.hpp>

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace sf {
class Font;
class RenderTarget;
} // namespace sf

namespace gfx {

class SfmlCanvas : public ICanvas {
public:
    explicit SfmlCanvas(sf::RenderTarget &target, type::SfmlType &);

    void set_viewport_size(int width, int height) override;
    constexpr void set_scale(int scale) override { scale_ = scale; }

    constexpr void add_translation(int dx, int dy) override {
        tx_ += dx;
        ty_ += dy;
    }

    void clear(Color) override;
    void draw_rect(geom::Rect const &, Color const &, Borders const &, Corners const &) override;
    void draw_text(geom::Position, std::string_view, std::span<Font const>, FontSize, FontStyle, Color) override;
    void draw_text(geom::Position, std::string_view, Font, FontSize, FontStyle, Color) override;
    void draw_pixels(geom::Rect const &, std::span<std::uint8_t const> rgba_data) override;

private:
    sf::RenderTarget &target_;
    type::SfmlType &type_;
    sf::Shader border_shader_{};
    std::vector<sf::Texture> textures_;

    int scale_{1};
    int tx_{0};
    int ty_{0};
};

} // namespace gfx

#endif
