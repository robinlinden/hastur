// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
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

auto get_font_dir_iterator(std::filesystem::path const &path) try {
    return std::filesystem::recursive_directory_iterator(path);
} catch (std::filesystem::filesystem_error const &) {
    return std::filesystem::recursive_directory_iterator();
}

// TODO(robinlinden): We should be looking at font names rather than filenames.
std::optional<std::string> find_path_to_font(std::string_view font_filename) {
    for (auto const &path : os::font_paths()) {
        for (auto const &entry : get_font_dir_iterator(path)) {
            auto name = entry.path().filename().string();
            if (name.find(font_filename) != std::string::npos) {
                std::cerr << "Found font " << entry.path().string() << " for \"" << font_filename << "\"\n";
                return std::make_optional(entry.path().string());
            }
        }
    }

    std::cerr << "Unable to find font " << font_filename << ", looking for literally any font\n";
    for (auto const &path : os::font_paths()) {
        for (auto const &entry : get_font_dir_iterator(path)) {
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

// TODO(robinlinden): Fonts are never evicted from the cache.
void SfmlPainter::draw_text(geom::Position p, std::string_view text, Font font, FontSize size, Color color) {
    auto const *sf_font = [&]() -> sf::Font const * {
        if (auto it = font_cache_.find(font.font); it != font_cache_.end()) {
            return &*it->second;
        }

        auto font_path = find_path_to_font(font.font);
        auto entry = std::make_shared<sf::Font>();
        if (!font_path || !entry->loadFromFile(*font_path)) {
            return nullptr;
        }

        font_cache_[std::string{font.font}] = std::move(entry);
        return &*font_cache_.find(font.font)->second;
    }();

    if (!sf_font) {
        std::cerr << "Unable to find font, not drawing text\n";
        return;
    }

    sf::Text drawable;
    drawable.setFont(*sf_font);
    drawable.setString(std::string{text});
    drawable.setFillColor(sf::Color(color.as_rgba_u32()));
    drawable.setCharacterSize(size.px);
    drawable.setStyle(sf::Text::Regular);
    drawable.setPosition(static_cast<float>(p.x + tx_), static_cast<float>(p.y + ty_));
    target_.draw(drawable);
}

} // namespace gfx
