// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "gfx/sfml_painter.h"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/View.hpp>

namespace gfx {

void SfmlPainter::set_viewport_size(int width, int height) {
    sf::View viewport{sf::FloatRect{0, 0, static_cast<float>(width), static_cast<float>(height)}};
    target_.setView(viewport);
}

void SfmlPainter::fill_rect(geom::Rect const &rect, Color color) {
    auto translated{rect.translated(tx_, ty_)};
    auto scaled{translated.scaled(scale_)};

    sf::RectangleShape drawable{{static_cast<float>(scaled.width), static_cast<float>(scaled.height)}};
    drawable.setPosition(static_cast<float>(scaled.x), static_cast<float>(scaled.y));
    drawable.setFillColor(sf::Color{color.r, color.g, color.b, color.a});
    target_.draw(drawable);
}

} // namespace gfx
