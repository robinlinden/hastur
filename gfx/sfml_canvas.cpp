// SPDX-FileCopyrightText: 2022-2024 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "gfx/sfml_canvas.h"

#include "geom/geom.h"
#include "gfx/color.h"
#include "gfx/font.h"
#include "gfx/icanvas.h"
#include "os/xdg.h"
#include "type/sfml.h"

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Glsl.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/System/String.hpp>
#include <spdlog/spdlog.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>

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

sf::Font load_fallback_font() {
    sf::Font font;
    for (auto const &path : os::font_paths()) {
        for (auto const &entry : get_font_dir_iterator(path)) {
            if (std::filesystem::is_regular_file(entry) && entry.path().filename().string().ends_with(".ttf")) {
                spdlog::info("Trying fallback {}", entry.path().string());
                if (font.openFromFile(entry.path().string())) {
                    spdlog::info("Using fallback {}", entry.path().string());
                    return font;
                }
            }
        }
    }

    spdlog::critical("Not a single usable font found");
    std::terminate();
}

std::shared_ptr<type::SfmlFont const> find_font(type::SfmlType &type, std::span<gfx::Font const> font_families) {
    for (auto const &family : font_families) {
        if (auto font = type.font(family.font)) {
            return std::static_pointer_cast<type::SfmlFont const>(*font);
        }
    }

    auto fallback = std::make_shared<type::SfmlFont>(load_fallback_font());
    if (!font_families.empty()) {
        type.set_font(std::string{font_families[0].font}, fallback);
    }

    return fallback;
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
    // NOLINTBEGIN(clang-analyzer-optin.core.EnumCastOutOfRange)
    auto sf_style = sf::Text::Style::Regular;
    if (style.bold) {
        sf_style = static_cast<sf::Text::Style>(sf_style | sf::Text::Style::Bold);
    }

    if (style.italic) {
        sf_style = static_cast<sf::Text::Style>(sf_style | sf::Text::Style::Italic);
    }

    if (style.underlined) {
        sf_style = static_cast<sf::Text::Style>(sf_style | sf::Text::Style::Underlined);
    }

    if (style.strikethrough) {
        sf_style = static_cast<sf::Text::Style>(sf_style | sf::Text::Style::StrikeThrough);
    }

    return sf_style;
    // NOLINTEND(clang-analyzer-optin.core.EnumCastOutOfRange)
}

} // namespace

SfmlCanvas::SfmlCanvas(sf::RenderTarget &target, type::SfmlType &type) : target_{target}, type_{type} {
    // TODO(robinlinden): Error-handling.
    std::ignore = border_shader_.loadFromMemory(
            std::string{reinterpret_cast<char const *>(gfx_basic_shader_vert), gfx_basic_shader_vert_len},
            std::string{reinterpret_cast<char const *>(gfx_rect_shader_frag), gfx_rect_shader_frag_len});
}

void SfmlCanvas::set_viewport_size(int width, int height) {
    sf::View viewport{sf::FloatRect{{0, 0}, {static_cast<float>(width), static_cast<float>(height)}}};
    target_.setView(viewport);
}

void SfmlCanvas::clear(Color c) {
    target_.clear(sf::Color(c.as_rgba_u32()));
    textures_.clear();
}

void SfmlCanvas::fill_rect(geom::Rect const &rect, Color color) {
    auto translated{rect.translated(tx_, ty_)};
    auto scaled{translated.scaled(scale_)};

    sf::RectangleShape drawable{{static_cast<float>(scaled.width), static_cast<float>(scaled.height)}};
    drawable.setPosition({static_cast<float>(scaled.x), static_cast<float>(scaled.y)});
    drawable.setFillColor(sf::Color{color.r, color.g, color.b, color.a});
    target_.draw(drawable);
}

void SfmlCanvas::draw_rect(geom::Rect const &rect, Color const &color, Borders const &borders, Corners const &corners) {
    auto translated{rect.translated(tx_, ty_)};
    auto inner_rect{translated.scaled(scale_)};
    auto outer_rect{
            inner_rect.expanded({borders.left.size, borders.right.size, borders.top.size, borders.bottom.size})};

    sf::RectangleShape drawable{{static_cast<float>(outer_rect.width), static_cast<float>(outer_rect.height)}};
    drawable.setPosition({static_cast<float>(outer_rect.x), static_cast<float>(outer_rect.y)});

    border_shader_.setUniform("resolution", target_.getView().getSize());

    border_shader_.setUniform("inner_top_left", to_vec2(inner_rect.left(), inner_rect.top()));
    border_shader_.setUniform("inner_top_right", to_vec2(inner_rect.right(), inner_rect.top()));
    border_shader_.setUniform("inner_bottom_left", to_vec2(inner_rect.left(), inner_rect.bottom()));
    border_shader_.setUniform("inner_bottom_right", to_vec2(inner_rect.right(), inner_rect.bottom()));

    border_shader_.setUniform("outer_top_left", to_vec2(outer_rect.left(), outer_rect.top()));
    border_shader_.setUniform("outer_top_right", to_vec2(outer_rect.right(), outer_rect.top()));
    border_shader_.setUniform("outer_bottom_left", to_vec2(outer_rect.left(), outer_rect.bottom()));
    border_shader_.setUniform("outer_bottom_right", to_vec2(outer_rect.right(), outer_rect.bottom()));

    border_shader_.setUniform("top_left_radii", to_vec2(corners.top_left.horizontal, corners.top_left.vertical));
    border_shader_.setUniform("top_right_radii", to_vec2(corners.top_right.horizontal, corners.top_right.vertical));
    border_shader_.setUniform(
            "bottom_left_radii", to_vec2(corners.bottom_left.horizontal, corners.bottom_left.vertical));
    border_shader_.setUniform(
            "bottom_right_radii", to_vec2(corners.bottom_right.horizontal, corners.bottom_right.vertical));

    border_shader_.setUniform("left_border_color", to_vec4(borders.left.color));
    border_shader_.setUniform("right_border_color", to_vec4(borders.right.color));
    border_shader_.setUniform("top_border_color", to_vec4(borders.top.color));
    border_shader_.setUniform("bottom_border_color", to_vec4(borders.bottom.color));
    border_shader_.setUniform("inner_rect_color", to_vec4(color));

    target_.draw(drawable, &border_shader_);
}

void SfmlCanvas::draw_text(geom::Position p,
        std::string_view text,
        std::span<Font const> font_options,
        FontSize size,
        FontStyle style,
        Color color) {
    p = p.translated(tx_, ty_).scaled(scale_);
    auto font = find_font(type_, font_options);
    assert(font != nullptr);

    sf::Text drawable{font->sf_font()};
    drawable.setString(sf::String::fromUtf8(cbegin(text), cend(text)));
    drawable.setFillColor(sf::Color(color.as_rgba_u32()));
    drawable.setCharacterSize(size.px * scale_);
    drawable.setStyle(to_sfml(style));
    drawable.setPosition({static_cast<float>(p.x), static_cast<float>(p.y)});
    target_.draw(drawable);
}

void SfmlCanvas::draw_text(
        geom::Position p, std::string_view text, Font font, FontSize size, FontStyle style, Color color) {
    draw_text(p, text, std::span<gfx::Font const>{{font}}, size, style, color);
}

void SfmlCanvas::draw_pixels(geom::Rect const &rect, std::span<std::uint8_t const> rgba_data) {
    assert(rgba_data.size() == static_cast<std::size_t>(rect.width * rect.height * 4));
    sf::Image img;
    // Textures need to be kept around while they're displayed. This will be
    // cleared when the canvas is cleared.
    sf::Texture &texture = textures_.emplace_back();
    if (!texture.resize({static_cast<unsigned>(rect.width), static_cast<unsigned>(rect.height)})) {
        spdlog::critical("Failed to resize texture");
        std::terminate();
    }

    texture.update(rgba_data.data());
    sf::Sprite sprite{texture};
    sprite.setPosition({static_cast<float>(rect.x), static_cast<float>(rect.y)});
    target_.draw(sprite);
    sf::RectangleShape shape{{static_cast<float>(rect.width), static_cast<float>(rect.height)}};
    shape.setTexture(&texture);
    target_.draw(shape);
}

} // namespace gfx
