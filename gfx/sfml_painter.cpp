// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "gfx/sfml_painter.h"

#include "os/os.h"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/View.hpp>

#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

namespace gfx {
namespace {

// TODO(robinlinden): We should be looking at font names rather than filenames.
std::optional<std::string> find_path_to_font(std::string_view font_filename) {
    for (auto const &path : os::font_paths()) {
        for (auto const &entry : std::filesystem::recursive_directory_iterator(path)) {
            auto name = entry.path().filename().string();
            if (name.find(font_filename) != std::string::npos) {
                return std::make_optional(entry.path().string());
            }
        }
    }

    std::cerr << "Unable to find font " << font_filename << ", looking for literally any font\n";
    for (auto const &path : os::font_paths()) {
        for (auto const &entry : std::filesystem::recursive_directory_iterator(path)) {
            if (std::filesystem::is_regular_file(entry) && entry.path().filename().string().ends_with(".ttf")) {
                std::cerr << "Using fallback " << entry.path().string() << '\n';
                return std::make_optional(entry.path().string());
            }
        }
    }

    std::cerr << "Unable to find fallback font\n";
    return std::nullopt;
}

} // namespace

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

void SfmlPainter::draw_text(geom::Position p, std::string_view text, Font font, FontSize size, Color color) {
    auto font_path = find_path_to_font(font.font);
    sf::Font sf_font;
    if (!font_path || !sf_font.loadFromFile(*font_path)) {
        std::cerr << "Unable to find font, not drawing text\n";
        return;
    }

    sf::Text drawable;
    drawable.setFont(sf_font);
    drawable.setString(std::string{text});
    drawable.setFillColor(sf::Color(color.as_rgba_u32()));
    drawable.setCharacterSize(size.px);
    drawable.setStyle(sf::Text::Regular);
    drawable.setPosition(static_cast<float>(p.x + tx_), static_cast<float>(p.y + ty_));
    target_.draw(drawable);
}

} // namespace gfx
