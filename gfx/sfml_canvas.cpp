// SPDX-FileCopyrightText: 2022-2023 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "gfx/sfml_canvas.h"

#include "os/os.h"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/View.hpp>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

using namespace std::literals;

namespace gfx {
namespace {

#include "gfx/basic_vertex_shader.h"
#include "gfx/rect_fragment_shader.h"

std::filesystem::recursive_directory_iterator get_font_dir_iterator(std::filesystem::path const &path) {
    std::error_code errc;
    if (auto it = std::filesystem::recursive_directory_iterator(path, errc); !errc) {
        return it;
    }

    return {};
}

std::optional<std::string> find_path_to_fallback_font() {
    for (auto const &path : os::font_paths()) {
        for (auto const &entry : get_font_dir_iterator(path)) {
            if (std::filesystem::is_regular_file(entry) && entry.path().filename().string().ends_with(".ttf")) {
                spdlog::info("Using fallback {}", entry.path().string());
                return std::make_optional(entry.path().string());
            }
        }
    }

    return std::nullopt;
}

// TODO(robinlinden): We should be looking at font names rather than filenames.
std::optional<std::string> find_path_to_font(std::string_view font_filename) {
    for (auto const &path : os::font_paths()) {
        for (auto const &entry : get_font_dir_iterator(path)) {
            auto name = entry.path().filename().string();
            if (name.find(font_filename) != std::string::npos) {
                spdlog::info("Found font {} for {}", entry.path().string(), font_filename);
                return std::make_optional(entry.path().string());
            }
        }
    }

    return std::nullopt;
}

sf::Glsl::Vec2 to_vec2(int x, int y) {
    return {static_cast<float>(x), static_cast<float>(y)};
}

sf::Glsl::Vec4 to_vec4(Color const &color) {
    return {static_cast<float>(color.r) / 0xFF,
            static_cast<float>(color.g) / 0xFF,
            static_cast<float>(color.b) / 0xFF,
            static_cast<float>(color.a) / 0xFF};
}

sf::Text::Style to_sfml(FontStyle style) {
    switch (style) {
        case FontStyle::Italic:
            return sf::Text::Italic;
        case FontStyle::Normal:
        default:
            return sf::Text::Regular;
    }
}

} // namespace

SfmlCanvas::SfmlCanvas(sf::RenderTarget &target) : target_{target} {
    border_shader_.loadFromMemory(
            std::string{reinterpret_cast<char const *>(gfx_basic_shader_vert), gfx_basic_shader_vert_len},
            std::string{reinterpret_cast<char const *>(gfx_rect_shader_frag), gfx_rect_shader_frag_len});
}

void SfmlCanvas::set_viewport_size(int width, int height) {
    sf::View viewport{sf::FloatRect{0, 0, static_cast<float>(width), static_cast<float>(height)}};
    target_.setView(viewport);
}

void SfmlCanvas::fill_rect(geom::Rect const &rect, Color color) {
    auto translated{rect.translated(tx_, ty_)};
    auto scaled{translated.scaled(scale_)};

    sf::RectangleShape drawable{{static_cast<float>(scaled.width), static_cast<float>(scaled.height)}};
    drawable.setPosition(static_cast<float>(scaled.x), static_cast<float>(scaled.y));
    drawable.setFillColor(sf::Color{color.r, color.g, color.b, color.a});
    target_.draw(drawable);
}

void SfmlCanvas::draw_rect(geom::Rect const &rect, Color const &color, Borders const &borders) {
    auto translated{rect.translated(tx_, ty_)};
    auto inner_rect{translated.scaled(scale_)};
    auto outer_rect{
            inner_rect.expanded({borders.left.size, borders.right.size, borders.top.size, borders.bottom.size})};

    sf::RectangleShape drawable{{static_cast<float>(outer_rect.width), static_cast<float>(outer_rect.height)}};
    drawable.setPosition(static_cast<float>(outer_rect.x), static_cast<float>(outer_rect.y));

    border_shader_.setUniform("resolution", target_.getView().getSize());

    border_shader_.setUniform("inner_top_left", to_vec2(inner_rect.left(), inner_rect.top()));
    border_shader_.setUniform("inner_top_right", to_vec2(inner_rect.right(), inner_rect.top()));
    border_shader_.setUniform("inner_bottom_left", to_vec2(inner_rect.left(), inner_rect.bottom()));
    border_shader_.setUniform("inner_bottom_right", to_vec2(inner_rect.right(), inner_rect.bottom()));

    border_shader_.setUniform("outer_top_left", to_vec2(outer_rect.left(), outer_rect.top()));
    border_shader_.setUniform("outer_top_right", to_vec2(outer_rect.right(), outer_rect.top()));
    border_shader_.setUniform("outer_bottom_left", to_vec2(outer_rect.left(), outer_rect.bottom()));
    border_shader_.setUniform("outer_bottom_right", to_vec2(outer_rect.right(), outer_rect.bottom()));

    // TODO(mkiael): Set radii when support for parsing it from stylesheet has been added
    border_shader_.setUniform("top_left_radii", to_vec2(0, 0));
    border_shader_.setUniform("top_right_radii", to_vec2(0, 0));
    border_shader_.setUniform("bottom_left_radii", to_vec2(0, 0));
    border_shader_.setUniform("bottom_right_radii", to_vec2(0, 0));

    border_shader_.setUniform("left_border_color", to_vec4(borders.left.color));
    border_shader_.setUniform("right_border_color", to_vec4(borders.right.color));
    border_shader_.setUniform("top_border_color", to_vec4(borders.top.color));
    border_shader_.setUniform("bottom_border_color", to_vec4(borders.bottom.color));
    border_shader_.setUniform("inner_rect_color", to_vec4(color));

    target_.draw(drawable, &border_shader_);
}

// TODO(robinlinden): Fonts are never evicted from the cache.
void SfmlCanvas::draw_text(
        geom::Position p, std::string_view text, Font font, FontSize size, FontStyle style, Color color) {
    auto const *sf_font = [&]() -> sf::Font const * {
        if (auto it = font_cache_.find(font.font); it != font_cache_.end()) {
            return &*it->second;
        }

        auto font_path = find_path_to_font(font.font);
        if (!font_path) {
            spdlog::warn("Unable to find font {}, looking for literally any font", font.font);
            font_path = find_path_to_fallback_font();
        }

        auto entry = std::make_shared<sf::Font>();
        if (!font_path || !entry->loadFromFile(*font_path)) {
            return nullptr;
        }

        font_cache_[std::string{font.font}] = std::move(entry);
        return &*font_cache_.find(font.font)->second;
    }();

    if (!sf_font) {
        spdlog::error("Unable to find font, not drawing text");
        return;
    }

    sf::Text drawable;
    drawable.setFont(*sf_font);
    drawable.setString(sf::String::fromUtf8(cbegin(text), cend(text)));
    drawable.setFillColor(sf::Color(color.as_rgba_u32()));
    drawable.setCharacterSize(size.px);
    drawable.setStyle(to_sfml(style));
    drawable.setPosition(static_cast<float>(p.x + tx_), static_cast<float>(p.y + ty_));
    target_.draw(drawable);
}

} // namespace gfx
