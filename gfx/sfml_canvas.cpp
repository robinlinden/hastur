// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
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

constexpr std::string_view basic_vertex_shader{
        R"(
        void main() {
           gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
           gl_FrontColor = gl_Color;
        }
        )"sv};

constexpr std::string_view border_fragment_shader{
        R"(
        uniform vec2 resolution;
        uniform vec3 inner_top_left;
        uniform vec3 inner_top_right;
        uniform vec3 inner_bottom_left;
        uniform vec3 inner_bottom_right;
        uniform vec3 outer_top_left;
        uniform vec3 outer_top_right;
        uniform vec3 outer_bottom_left;
        uniform vec3 outer_bottom_right;
        uniform vec4 left_border_color;
        uniform vec4 right_border_color;
        uniform vec4 top_border_color;
        uniform vec4 bottom_border_color;

        // Gets the position of the fragment in screen coordinates
        vec3 get_frag_pos() {
            vec2 p = vec2(gl_FragCoord.x, resolution.y - gl_FragCoord.y);
            return vec3(p.x, p.y, 0.0);
        }

        // Checks if a point is inside a quadrilateral
        bool is_point_inside(vec3 p, vec3 a, vec3 b, vec3 c, vec3 d) {
            return dot(cross(p - a, b - a), cross(p - d, c - d)) <= 0.0 &&
                   dot(cross(p - a, d - a), cross(p - b, c - b)) <= 0.0;
        }

        void main() {
            vec3 p = get_frag_pos();
            if (is_point_inside(p, inner_top_left, inner_top_right, inner_bottom_right, inner_bottom_left)) {
                // The fragment is not on a border, set fully transparent color
                gl_FragColor = vec4(1.0, 1.0, 1.0, 0.0);
            } else {
                if (is_point_inside(p, outer_top_left, outer_top_right, inner_top_right, inner_top_left)) {
                    gl_FragColor = top_border_color;
                } else if (is_point_inside(p, outer_top_right, outer_bottom_right, inner_bottom_right, inner_top_right)) {
                    gl_FragColor = right_border_color;
                } else if (is_point_inside(p, outer_bottom_right, outer_bottom_left, inner_bottom_left, inner_bottom_right)) {
                    gl_FragColor = bottom_border_color;
                } else if (is_point_inside(p, outer_bottom_left, outer_top_left, inner_top_left, inner_bottom_left)) {
                    gl_FragColor = left_border_color;
                } else {
                    // The fragment is not on a border, set fully transparent color
                    gl_FragColor = vec4(1.0, 1.0, 1.0, 0.0);
                }
            }
        }
        )"sv};

std::filesystem::recursive_directory_iterator get_font_dir_iterator(std::filesystem::path const &path) {
    std::error_code errc;
    if (auto it = std::filesystem::recursive_directory_iterator(path, errc); !errc) {
        return it;
    }

    return {};
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

    spdlog::warn("Unable to find font {}, looking for literally any font", font_filename);
    for (auto const &path : os::font_paths()) {
        for (auto const &entry : get_font_dir_iterator(path)) {
            if (std::filesystem::is_regular_file(entry) && entry.path().filename().string().ends_with(".ttf")) {
                spdlog::info("Using fallback {}", entry.path().string());
                return std::make_optional(entry.path().string());
            }
        }
    }

    spdlog::error("Unable to find fallback font");
    return std::nullopt;
}

sf::Glsl::Vec3 to_vec3(int x, int y) {
    return sf::Glsl::Vec3(static_cast<float>(x), static_cast<float>(y), 0.0);
}

sf::Glsl::Vec4 to_vec4(Color const &color) {
    return sf::Glsl::Vec4(static_cast<float>(color.r) / 0xFF,
            static_cast<float>(color.g) / 0xFF,
            static_cast<float>(color.b) / 0xFF,
            static_cast<float>(color.a) / 0xFF);
}

} // namespace

SfmlCanvas::SfmlCanvas(sf::RenderTarget &target) : target_{target} {
    border_shader_.loadFromMemory(std::string{basic_vertex_shader}, std::string{border_fragment_shader});
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

void SfmlCanvas::draw_border(geom::Rect const &rect, Borders const &borders) {
    auto translated{rect.translated(tx_, ty_)};
    auto inner_rect{translated.scaled(scale_)};
    auto outer_rect =
            inner_rect.expanded({borders.left.size, borders.right.size, borders.top.size, borders.bottom.size});

    sf::RectangleShape drawable{{static_cast<float>(outer_rect.width), static_cast<float>(outer_rect.height)}};
    drawable.setPosition(static_cast<float>(outer_rect.x), static_cast<float>(outer_rect.y));

    border_shader_.setUniform("resolution", target_.getView().getSize());

    border_shader_.setUniform("inner_top_left", to_vec3(inner_rect.left(), inner_rect.top()));
    border_shader_.setUniform("inner_top_right", to_vec3(inner_rect.right(), inner_rect.top()));
    border_shader_.setUniform("inner_bottom_left", to_vec3(inner_rect.left(), inner_rect.bottom()));
    border_shader_.setUniform("inner_bottom_right", to_vec3(inner_rect.right(), inner_rect.bottom()));

    border_shader_.setUniform("outer_top_left", to_vec3(outer_rect.left(), outer_rect.top()));
    border_shader_.setUniform("outer_top_right", to_vec3(outer_rect.right(), outer_rect.top()));
    border_shader_.setUniform("outer_bottom_left", to_vec3(outer_rect.left(), outer_rect.bottom()));
    border_shader_.setUniform("outer_bottom_right", to_vec3(outer_rect.right(), outer_rect.bottom()));

    border_shader_.setUniform("left_border_color", to_vec4(borders.left.color));
    border_shader_.setUniform("right_border_color", to_vec4(borders.right.color));
    border_shader_.setUniform("top_border_color", to_vec4(borders.top.color));
    border_shader_.setUniform("bottom_border_color", to_vec4(borders.bottom.color));

    target_.draw(drawable, &border_shader_);
}

// TODO(robinlinden): Fonts are never evicted from the cache.
void SfmlCanvas::draw_text(geom::Position p, std::string_view text, Font font, FontSize size, Color color) {
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
        spdlog::error("Unable to find font, not drawing text");
        return;
    }

    sf::Text drawable;
    drawable.setFont(*sf_font);
    drawable.setString(sf::String::fromUtf8(cbegin(text), cend(text)));
    drawable.setFillColor(sf::Color(color.as_rgba_u32()));
    drawable.setCharacterSize(size.px);
    drawable.setStyle(sf::Text::Regular);
    drawable.setPosition(static_cast<float>(p.x + tx_), static_cast<float>(p.y + ty_));
    target_.draw(drawable);
}

} // namespace gfx
