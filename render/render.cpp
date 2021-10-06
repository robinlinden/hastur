// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "render/render.h"

#include "dom/dom.h"
#include "style/style.h"

// MSVC gl.h doesn't include everything it uses.
#ifdef _MSC_VER
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // _MSC_VER

#include <GL/gl.h>
#include <spdlog/spdlog.h>

#include <charconv>
#include <cstdint>
#include <variant>

namespace render {
namespace {

bool looks_like_hex(std::string_view str) {
    return str.starts_with('#') && str.length() == 7;
}

bool contains_text(layout::LayoutBox const &layout) {
    return std::holds_alternative<dom::Text>(layout.node->node.get().data);
}

gfx::Color from_hex_chars(std::string_view hex_chars) {
    hex_chars.remove_prefix(1);
    std::int32_t hex{};
    std::from_chars(hex_chars.data(), hex_chars.data() + hex_chars.size(), hex, /*base*/ 16);
    return gfx::Color::from_rgb(hex);
}

gfx::Color parse_color(std::string_view str) {
    if (looks_like_hex(str)) {
        return from_hex_chars(str);
    }

    spdlog::warn("Unrecognized color format: {}", str);
    return gfx::Color{0xFF, 0, 0};
}

void do_render(gfx::IPainter &painter, layout::LayoutBox const &layout) {
    if (contains_text(layout)) {
        auto color = style::get_property_or(*layout.node, "color", "#000000");
        painter.fill_rect(layout.dimensions.padding_box(), parse_color(color));
    } else {
        if (auto maybe_color = style::get_property(*layout.node, "background-color")) {
            painter.fill_rect(layout.dimensions.padding_box(), parse_color(*maybe_color));
        }
    }
}

bool should_render(layout::LayoutBox const &layout) {
    return layout.type == layout::LayoutType::Block || layout.type == layout::LayoutType::Inline;
}

} // namespace

void render_setup(int width, int height) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1.0, 1.0);
    glViewport(0, 0, width, height);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void render_layout(gfx::IPainter &painter, layout::LayoutBox const &layout) {
    if (should_render(layout)) {
        do_render(painter, layout);
    }

    for (auto const &child : layout.children) {
        render_layout(painter, child);
    }
}

namespace debug {
namespace {
void render_layout_depth_impl(gfx::IPainter &painter, layout::LayoutBox const &layout, int depth) {
    auto color = static_cast<std::uint8_t>(255 / depth);
    painter.fill_rect(layout.dimensions.padding_box(), {color, color, color});
    for (auto const &child : layout.children) {
        render_layout_depth_impl(painter, child, depth + 1);
    }
}
} // namespace

void render_layout_depth(gfx::IPainter &painter, layout::LayoutBox const &layout) {
    render_layout_depth_impl(painter, layout, 1);
}

} // namespace debug
} // namespace render
