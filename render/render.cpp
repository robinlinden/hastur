// SPDX-FileCopyrightText: 2021-2025 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "render/render.h"

#include "css/property_id.h"
#include "dom/dom.h"
#include "dom/xpath.h"
#include "geom/geom.h"
#include "gfx/color.h"
#include "gfx/font.h"
#include "gfx/icanvas.h"
#include "layout/layout_box.h"
#include "style/styled_node.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <optional>
#include <string_view>
#include <variant>
#include <vector>

namespace render {
namespace {

bool has_any_border(geom::EdgeSize const &border) {
    return border != geom::EdgeSize{};
}

constexpr bool is_fully_transparent(gfx::Color const &c) {
    return c.a == 0;
}

gfx::FontStyle to_gfx(style::FontStyle style,
        std::optional<style::FontWeight> weight,
        std::vector<style::TextDecorationLine> const &decorations) {
    gfx::FontStyle gfx;
    if (style == style::FontStyle::Italic || style == style::FontStyle::Oblique) {
        gfx.italic = true;
    }

    if (weight && weight->value >= style::FontWeight::kBold) {
        gfx.bold = true;
    }

    for (auto const &decoration : decorations) {
        switch (decoration) {
            case style::TextDecorationLine::None:
                break;
            case style::TextDecorationLine::Underline:
                gfx.underlined = true;
                break;
            case style::TextDecorationLine::LineThrough:
                gfx.strikethrough = true;
                break;
            case style::TextDecorationLine::Overline:
                gfx.overlined = true;
                break;
        }
    }

    return gfx;
}

void render_text(gfx::ICanvas &painter, layout::LayoutBox const &layout, std::string_view text) {
    auto font_families = layout.get_property<css::PropertyId::FontFamily>();
    auto fonts = [&font_families] {
        std::vector<gfx::Font> fs;
        std::ranges::transform(font_families, std::back_inserter(fs), [](auto f) { return gfx::Font{f}; });
        return fs;
    }();
    auto font_size = gfx::FontSize{.px = layout.get_property<css::PropertyId::FontSize>()};
    auto style = to_gfx(layout.get_property<css::PropertyId::FontStyle>(),
            layout.get_property<css::PropertyId::FontWeight>(),
            layout.get_property<css::PropertyId::TextDecorationLine>());
    auto color = layout.get_property<css::PropertyId::Color>();
    painter.draw_text(layout.dimensions.content.position(), text, fonts, font_size, style, color);
}

void render_element(gfx::ICanvas &painter, layout::LayoutBox const &layout) {
    auto background_color = layout.get_property<css::PropertyId::BackgroundColor>();
    auto const &border_size = layout.dimensions.border;

    gfx::Corners corners{};
    auto top_left = layout.get_property<css::PropertyId::BorderTopLeftRadius>();
    corners.top_left = {top_left.first, top_left.second};
    auto top_right = layout.get_property<css::PropertyId::BorderTopRightRadius>();
    corners.top_right = {top_right.first, top_right.second};
    auto bottom_left = layout.get_property<css::PropertyId::BorderBottomLeftRadius>();
    corners.bottom_left = {bottom_left.first, bottom_left.second};
    auto bottom_right = layout.get_property<css::PropertyId::BorderBottomRightRadius>();
    corners.bottom_right = {bottom_right.first, bottom_right.second};

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

        painter.draw_rect(layout.dimensions.padding_box(), background_color, borders, corners);
    } else if (!is_fully_transparent(background_color)) {
        painter.draw_rect(layout.dimensions.padding_box(), background_color, gfx::Borders{}, corners);
    }
}

void render_image(gfx::ICanvas &painter, layout::LayoutBox const &layout, ImageView const &image) {
    // TODO(robinlinden): Handle image scaling. image.{width,height} are unused
    // right now, but should be used to scale the image to work with the content
    // size.
    painter.draw_pixels(layout.dimensions.content, image.rgba_data);
}

std::optional<std::string_view> get_image_id(layout::LayoutBox const &layout) {
    assert(!layout.is_anonymous_block());
    auto const *img = std::get_if<dom::Element>(&layout.node->node);
    if (img == nullptr || img->name != "img") {
        return {};
    }

    auto src = img->attributes.find("src");
    if (src == img->attributes.end()) {
        return {};
    }

    return src->second;
}

void do_render(gfx::ICanvas &painter, layout::LayoutBox const &layout, ImageLookupFn const &image_lookup) {
    if (auto text = layout.text()) {
        render_text(painter, layout, *text);
    } else if (auto img = get_image_id(layout).and_then(image_lookup); img.has_value()) {
        render_image(painter, layout, *img);
    } else {
        render_element(painter, layout);
    }
}

bool should_render(layout::LayoutBox const &layout) {
    return !layout.is_anonymous_block();
}

// NOLINTNEXTLINE(misc-no-recursion)
void render_layout_impl(gfx::ICanvas &painter,
        layout::LayoutBox const &layout,
        std::optional<geom::Rect> const &clip,
        ImageLookupFn const &image_lookup) {
    if (clip && clip->intersected(layout.dimensions.border_box()).empty()) {
        return;
    }

    if (should_render(layout)) {
        do_render(painter, layout, image_lookup);
    }

    for (auto const &child : layout.children) {
        render_layout_impl(painter, child, clip, image_lookup);
    }
}

} // namespace

void render_layout(gfx::ICanvas &painter,
        layout::LayoutBox const &layout,
        std::optional<geom::Rect> const &clip,
        ImageLookupFn const &image_lookup) {
    static constexpr auto kGetBg = [](std::string_view xpath, layout::LayoutBox const &l) -> std::optional<gfx::Color> {
        auto d = dom::nodes_by_xpath(l, xpath);
        if (d.empty()) {
            return std::nullopt;
        }

        return d[0]->get_property<css::PropertyId::BackgroundColor>();
    };

    // https://www.w3.org/TR/css-backgrounds-3/#special-backgrounds
    // If html or body has a background set, use that as the canvas background.
    if (auto html_bg = kGetBg("/html", layout); html_bg && html_bg != gfx::Color::from_css_name("transparent")) {
        painter.clear(*html_bg);
    } else if (auto body_bg = kGetBg("/html/body", layout);
            body_bg && body_bg != gfx::Color::from_css_name("transparent")) {
        painter.clear(*body_bg);
    } else {
        painter.clear(gfx::Color{255, 255, 255});
    }

    render_layout_impl(painter, layout, clip, image_lookup);
}

namespace debug {
namespace {

// NOLINTNEXTLINE(misc-no-recursion)
void render_layout_depth_impl(gfx::ICanvas &painter, layout::LayoutBox const &layout) {
    painter.draw_rect(layout.dimensions.padding_box(), {0xFF, 0xFF, 0xFF, 0x30}, {}, {});
    for (auto const &child : layout.children) {
        render_layout_depth_impl(painter, child);
    }
}

} // namespace

void render_layout_depth(gfx::ICanvas &painter, layout::LayoutBox const &layout) {
    painter.clear(gfx::Color{});
    render_layout_depth_impl(painter, layout);
}

} // namespace debug
} // namespace render
