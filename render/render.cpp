// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "render/render.h"

#include "css/property_id.h"
#include "dom/dom.h"
#include "gfx/color.h"
#include "util/from_chars.h"
#include "util/string.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <string_view>
#include <variant>

using namespace std::literals;

namespace render {
namespace {

bool has_any_border(geom::EdgeSize const &border) {
    return border != geom::EdgeSize{};
}

dom::Text const *try_get_text(layout::LayoutBox const &layout) {
    return std::get_if<dom::Text>(&layout.node->node);
}

constexpr bool is_fully_transparent(gfx::Color const &c) {
    return c.a == 0;
}

gfx::FontStyle to_gfx(style::FontStyle style) {
    switch (style) {
        case style::FontStyle::Italic:
        case style::FontStyle::Oblique:
            return gfx::FontStyle::Italic;
        case style::FontStyle::Normal:
        default:
            return gfx::FontStyle::Normal;
    }
}

void render_text(gfx::Painter &painter, layout::LayoutBox const &layout, dom::Text const &text) {
    auto font_families = layout.get_property<css::PropertyId::FontFamily>();
    auto fonts = [&font_families] {
        std::vector<gfx::Font> fs;
        std::ranges::transform(font_families, std::back_inserter(fs), [](auto f) { return gfx::Font{f}; });
        return fs;
    }();
    auto font_size = gfx::FontSize{.px = layout.get_property<css::PropertyId::FontSize>()};
    auto style = layout.get_property<css::PropertyId::FontStyle>();
    auto color = layout.get_property<css::PropertyId::Color>();
    painter.draw_text(layout.dimensions.content.position(), text.text, fonts, font_size, to_gfx(style), color);
}

void render_element(gfx::Painter &painter, layout::LayoutBox const &layout) {
    auto background_color = layout.get_property<css::PropertyId::BackgroundColor>();
    auto const &border_size = layout.dimensions.border;
    if (has_any_border(border_size)) {
        gfx::Borders borders{};
        borders.left.color = layout.get_property<css::PropertyId::BorderLeftColor>();
        borders.left.size = border_size.left;
        borders.right.color = layout.get_property<css::PropertyId::BorderRightColor>();
        borders.right.size = border_size.right;
        borders.top.color = layout.get_property<css::PropertyId::BorderTopColor>();
        borders.top.size = border_size.top;
        borders.bottom.color = layout.get_property<css::PropertyId::BorderBottomColor>();
        borders.bottom.size = border_size.bottom;

        painter.draw_rect(layout.dimensions.padding_box(), background_color, borders, gfx::Corners{});
    } else if (!is_fully_transparent(background_color)) {
        painter.draw_rect(layout.dimensions.padding_box(), background_color, gfx::Borders{}, gfx::Corners{});
    }
}

void do_render(gfx::Painter &painter, layout::LayoutBox const &layout) {
    if (auto const *text = try_get_text(layout)) {
        render_text(painter, layout, *text);
    } else {
        render_element(painter, layout);
    }
}

bool should_render(layout::LayoutBox const &layout) {
    return layout.type == layout::LayoutType::Block || layout.type == layout::LayoutType::Inline;
}

} // namespace

void render_layout(gfx::Painter &painter, layout::LayoutBox const &layout) {
    if (should_render(layout)) {
        do_render(painter, layout);
    }

    for (auto const &child : layout.children) {
        render_layout(painter, child);
    }
}

namespace debug {

void render_layout_depth(gfx::Painter &painter, layout::LayoutBox const &layout) {
    painter.fill_rect(layout.dimensions.padding_box(), {0xFF, 0xFF, 0xFF, 0x30});
    for (auto const &child : layout.children) {
        render_layout_depth(painter, child);
    }
}

} // namespace debug
} // namespace render
