// SPDX-FileCopyrightText: 2021-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef RENDER_RENDER_H_
#define RENDER_RENDER_H_

#include "geom/geom.h"
#include "gfx/icanvas.h"
#include "layout/layout_box.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string_view>

namespace render {

struct ImageView {
    std::uint32_t width{};
    std::uint32_t height{};
    std::span<std::uint8_t const> rgba_data;
};

using ImageLookupFn = std::function<std::optional<ImageView>(std::string_view id)>;

void render_layout(
        gfx::ICanvas &,
        layout::LayoutBox const &,
        std::optional<geom::Rect> const &clip = std::nullopt,
        ImageLookupFn const & = [](auto) { return std::nullopt; });

namespace debug {
void render_layout_depth(gfx::ICanvas &, layout::LayoutBox const &);
} // namespace debug
} // namespace render

#endif
