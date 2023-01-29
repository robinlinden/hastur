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

constexpr gfx::Color kDefaultColor{0x0, 0x0, 0x0};

bool has_any_border(geom::EdgeSize const &border) {
    return border != geom::EdgeSize{};
}

dom::Text const *try_get_text(layout::LayoutBox const &layout) {
    return std::get_if<dom::Text>(&layout.node->node);
}

constexpr bool is_fully_transparent(gfx::Color const &c) {
    return c.a == 0;
}

std::optional<gfx::Color> try_from_hex_chars(std::string_view hex_chars) {
    if (!hex_chars.starts_with('#')) {
        return std::nullopt;
    }

    hex_chars.remove_prefix(1);
    std::uint32_t hex{};
    if (hex_chars.length() == 6) {
        std::from_chars(hex_chars.data(), hex_chars.data() + hex_chars.size(), hex, /*base*/ 16);
        return gfx::Color::from_rgb(hex);
    } else if (hex_chars.length() == 3) {
        std::ostringstream ss;
        ss << hex_chars[0] << hex_chars[0] << hex_chars[1] << hex_chars[1] << hex_chars[2] << hex_chars[2];
        auto expanded = std::move(ss).str();
        std::from_chars(expanded.data(), expanded.data() + expanded.size(), hex, /*base*/ 16);
        return gfx::Color::from_rgb(hex);
    } else if (hex_chars.length() == 8) {
        std::from_chars(hex_chars.data(), hex_chars.data() + hex_chars.size(), hex, /*base*/ 16);
        return gfx::Color::from_rgba(hex);
    } else if (hex_chars.length() == 4) {
        std::ostringstream ss;
        ss << hex_chars[0] << hex_chars[0] << hex_chars[1] << hex_chars[1] << hex_chars[2] << hex_chars[2]
           << hex_chars[3] << hex_chars[3];
        auto expanded = std::move(ss).str();
        std::from_chars(expanded.data(), expanded.data() + expanded.size(), hex, /*base*/ 16);
        return gfx::Color::from_rgba(hex);
    }

    return std::nullopt;
}

// TODO(robinlinden): space-separated values.
// https://developer.mozilla.org/en-US/docs/Web/CSS/color_value/rgb
// https://developer.mozilla.org/en-US/docs/Web/CSS/color_value/rgba
std::optional<gfx::Color> try_from_rgba(std::string_view text) {
    if (text.starts_with("rgb(")) {
        text.remove_prefix(std::strlen("rgb("));
    } else if (text.starts_with("rgba(")) {
        text.remove_prefix(std::strlen("rgba("));
    } else {
        return std::nullopt;
    }

    if (!text.ends_with(')')) {
        return std::nullopt;
    }
    text.remove_suffix(std::strlen(")"));

    auto rgba = util::split(text, ",");
    if (rgba.size() != 3 && rgba.size() != 4) {
        return std::nullopt;
    }

    for (auto &value : rgba) {
        value = util::trim(value);
    }

    auto to_int = [](std::string_view v) {
        int ret{-1};
        if (std::from_chars(v.data(), v.data() + v.size(), ret).ptr != v.data() + v.size()) {
            return -1;
        }

        if (ret < 0 || ret > 255) {
            return -1;
        }

        return ret;
    };

    auto r{to_int(rgba[0])};
    auto g{to_int(rgba[1])};
    auto b{to_int(rgba[2])};
    if (r == -1 || g == -1 || b == -1) {
        return std::nullopt;
    }

    if (rgba.size() == 3) {
        return gfx::Color{static_cast<std::uint8_t>(r), static_cast<std::uint8_t>(g), static_cast<std::uint8_t>(b)};
    }

    float a{-1.f};
    if (util::from_chars(rgba[3].data(), rgba[3].data() + rgba[3].size(), a).ptr != rgba[3].data() + rgba[3].size()) {
        return std::nullopt;
    }

    if (a < 0.f || a > 1.f) {
        return std::nullopt;
    }

    return gfx::Color{
            static_cast<std::uint8_t>(r),
            static_cast<std::uint8_t>(g),
            static_cast<std::uint8_t>(b),
            static_cast<std::uint8_t>(a * 255),
    };
}

gfx::Color parse_color(std::string_view str) {
    if (auto color = try_from_hex_chars(str)) {
        return *color;
    }

    if (auto color = try_from_rgba(str)) {
        return *color;
    }

    if (auto css_named_color = gfx::Color::from_css_name(str)) {
        return *css_named_color;
    }

    spdlog::warn("Unrecognized color format: {}", str);
    return gfx::Color{0xFF, 0, 0};
}

template<css::PropertyId T>
std::optional<gfx::Color> try_get_color(layout::LayoutBox const &layout) {
    auto maybe_color = layout.get_property<T>();

    // https://developer.mozilla.org/en-US/docs/Web/CSS/color_value#currentcolor_keyword
    if (maybe_color == "currentcolor") {
        return layout.get_property<css::PropertyId::Color>();
    }

    if (maybe_color) {
        return parse_color(*maybe_color);
    }

    return std::nullopt;
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
        std::ranges::transform(font_families.value(), std::back_inserter(fs), [](auto f) { return gfx::Font{f}; });
        return fs;
    }();
    auto font_size = gfx::FontSize{.px = layout.get_property<css::PropertyId::FontSize>().value()};
    auto style = layout.get_property<css::PropertyId::FontStyle>().value_or(style::FontStyle::Normal);
    auto color = layout.get_property<css::PropertyId::Color>().value();
    painter.draw_text(layout.dimensions.content.position(), text.text, fonts, font_size, to_gfx(style), color);
}

void render_element(gfx::Painter &painter, layout::LayoutBox const &layout) {
    auto background_color = layout.get_property<css::PropertyId::BackgroundColor>().value();
    auto const &border_size = layout.dimensions.border;
    if (has_any_border(border_size)) {
        gfx::Borders borders{};
        borders.left.color = layout.get_property<css::PropertyId::BorderLeftColor>().value_or(kDefaultColor);
        borders.left.size = border_size.left;
        borders.right.color = layout.get_property<css::PropertyId::BorderRightColor>().value_or(kDefaultColor);
        borders.right.size = border_size.right;
        borders.top.color = layout.get_property<css::PropertyId::BorderTopColor>().value_or(kDefaultColor);
        borders.top.size = border_size.top;
        borders.bottom.color = layout.get_property<css::PropertyId::BorderBottomColor>().value_or(kDefaultColor);
        borders.bottom.size = border_size.bottom;

        painter.draw_rect(layout.dimensions.padding_box(), background_color, borders);
    } else if (!is_fully_transparent(background_color)) {
        painter.draw_rect(layout.dimensions.padding_box(), background_color, gfx::Borders{});
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
