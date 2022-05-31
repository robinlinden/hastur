// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "render/render.h"

#include "dom/dom.h"
#include "gfx/color.h"
#include "style/style.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <map>
#include <sstream>
#include <string_view>
#include <variant>

using namespace std::literals;

namespace render {
namespace {

constexpr std::string_view kDefaultColor{"#000000"};

struct CaseInsensitiveLess {
    using is_transparent = void;
    bool operator()(std::string_view s1, std::string_view s2) const {
        return std::ranges::lexicographical_compare(
                s1, s2, [](unsigned char c1, unsigned char c2) { return std::tolower(c1) < std::tolower(c2); });
    }
};

// https://developer.mozilla.org/en-US/docs/Web/CSS/color_value/color_keywords#list_of_all_color_keywords
std::map<std::string_view, gfx::Color, CaseInsensitiveLess> named_colors{
        // Special.
        // https://developer.mozilla.org/en-US/docs/Web/CSS/color_value#transparent_keyword
        {"transparent", {0x00, 0x00, 0x00, 0x00}},
        // CSS Level 1.
        {"black", gfx::Color::from_rgb(0)},
        {"silver", gfx::Color::from_rgb(0xc0'c0'c0)},
        {"gray", gfx::Color::from_rgb(0x80'80'80)},
        {"white", gfx::Color::from_rgb(0xff'ff'ff)},
        {"maroon", gfx::Color::from_rgb(0x80'00'00)},
        {"red", gfx::Color::from_rgb(0xff'00'00)},
        {"purple", gfx::Color::from_rgb(0x80'00'80)},
        {"fuchsia", gfx::Color::from_rgb(0xff'00'ff)},
        {"green", gfx::Color::from_rgb(0x00'80'00)},
        {"lime", gfx::Color::from_rgb(0x00'ff'00)},
        {"olive", gfx::Color::from_rgb(0x80'80'00)},
        {"yellow", gfx::Color::from_rgb(0xff'ff'00)},
        {"navy", gfx::Color::from_rgb(0x00'00'80)},
        {"blue", gfx::Color::from_rgb(0x00'00'ff)},
        {"teal", gfx::Color::from_rgb(0x00'80'80)},
        {"aqua", gfx::Color::from_rgb(0x00'ff'ff)},
};

bool looks_like_hex(std::string_view str) {
    return str.starts_with('#') && (str.length() == 7 || str.length() == 4);
}

bool has_any_border(layout::LayoutBox const &layout) {
    return layout.dimensions.border_box() != layout.dimensions.padding_box();
}

dom::Text const *try_get_text(layout::LayoutBox const &layout) {
    return std::get_if<dom::Text>(&layout.node->node);
}

gfx::Color from_hex_chars(std::string_view hex_chars) {
    hex_chars.remove_prefix(1);
    std::int32_t hex{};
    if (hex_chars.length() == 6) {
        std::from_chars(hex_chars.data(), hex_chars.data() + hex_chars.size(), hex, /*base*/ 16);
    } else {
        std::ostringstream ss;
        ss << hex_chars[0] << hex_chars[0] << hex_chars[1] << hex_chars[1] << hex_chars[2] << hex_chars[2];
        auto expanded = ss.str();
        std::from_chars(expanded.data(), expanded.data() + expanded.size(), hex, /*base*/ 16);
    }

    return gfx::Color::from_rgb(hex);
}

gfx::Color parse_color(std::string_view str) {
    if (looks_like_hex(str)) {
        return from_hex_chars(str);
    }

    if (named_colors.contains(str)) {
        return named_colors.at(str);
    }

    spdlog::warn("Unrecognized color format: {}", str);
    return gfx::Color{0xFF, 0, 0};
}

void render_text(gfx::IPainter &painter, layout::LayoutBox const &layout, dom::Text const &text) {
    // TODO(robinlinden):
    // * We need to grab properties from the parent for this to work.
    // * This shouldn't be done here.
    auto font = gfx::Font{"arial"sv};
    auto font_size = gfx::FontSize{.px = 10};
    auto color = parse_color(style::get_property(*layout.node, "color"sv).value_or(kDefaultColor));
    painter.draw_text(layout.dimensions.content.position(), text.text, font, font_size, color);
}

void render_borders(gfx::IPainter &painter, layout::LayoutBox const &layout) {
    // TODO(mkiael): Handle a lot more border styles
    auto color = style::get_property(*layout.node, "color"sv).value_or(kDefaultColor);
    auto border_box = layout.dimensions.border_box();
    auto const &border_size = layout.dimensions.border;
    if (border_size.left > 0) {
        geom::Rect border{border_box.left(),
                border_box.top() + border_size.top,
                border_size.left,
                border_box.height - border_size.top - border_size.bottom};
        auto border_color = style::get_property(*layout.node, "border-left-color"sv).value_or(color);
        painter.fill_rect(border, parse_color(border_color));
    }
    if (border_size.right > 0) {
        geom::Rect border{border_box.right() - border_size.right,
                border_box.top() + border_size.top,
                border_size.right,
                border_box.height - border_size.top - border_size.bottom};
        auto border_color = style::get_property(*layout.node, "border-right-color"sv).value_or(color);
        painter.fill_rect(border, parse_color(border_color));
    }
    if (border_size.top > 0) {
        geom::Rect border{border_box.left(), border_box.top(), border_box.width, border_size.top};
        auto border_color = style::get_property(*layout.node, "border-top-color"sv).value_or(color);
        painter.fill_rect(border, parse_color(border_color));
    }
    if (border_size.bottom > 0) {
        geom::Rect border{
                border_box.left(), border_box.bottom() - border_size.bottom, border_box.width, border_size.bottom};
        auto border_color = style::get_property(*layout.node, "border-bottom-color"sv).value_or(color);
        painter.fill_rect(border, parse_color(border_color));
    }
}

void render_background(gfx::IPainter &painter, layout::LayoutBox const &layout) {
    if (auto maybe_color = style::get_property(*layout.node, "background-color")) {
        painter.fill_rect(layout.dimensions.padding_box(), parse_color(*maybe_color));
    }
}

void do_render(gfx::IPainter &painter, layout::LayoutBox const &layout) {
    if (auto const *text = try_get_text(layout)) {
        render_text(painter, layout, *text);
    } else {
        if (has_any_border(layout)) {
            render_borders(painter, layout);
        }
        render_background(painter, layout);
    }
}

bool should_render(layout::LayoutBox const &layout) {
    return layout.type == layout::LayoutType::Block || layout.type == layout::LayoutType::Inline;
}

} // namespace

void render_layout(gfx::IPainter &painter, layout::LayoutBox const &layout) {
    if (should_render(layout)) {
        do_render(painter, layout);
    }

    for (auto const &child : layout.children) {
        render_layout(painter, child);
    }
}

namespace debug {

void render_layout_depth(gfx::IPainter &painter, layout::LayoutBox const &layout) {
    painter.fill_rect(layout.dimensions.padding_box(), {0xFF, 0xFF, 0xFF, 0x30});
    for (auto const &child : layout.children) {
        render_layout_depth(painter, child);
    }
}

} // namespace debug
} // namespace render
