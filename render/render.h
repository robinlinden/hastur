// SPDX-FileCopyrightText: 2021-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef RENDER_RENDER_H_
#define RENDER_RENDER_H_

#include "geom/geom.h"
#include "gfx/icanvas.h"
#include "layout/layout_box.h"

#include <optional>

namespace render {

void render_layout(gfx::ICanvas &, layout::LayoutBox const &, std::optional<geom::Rect> const &clip = std::nullopt);

namespace debug {
void render_layout_depth(gfx::ICanvas &, layout::LayoutBox const &);
} // namespace debug
} // namespace render

#endif
