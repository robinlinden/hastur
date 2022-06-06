// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef RENDER_RENDER_H_
#define RENDER_RENDER_H_

#include "gfx/painter.h"
#include "layout/layout.h"

namespace render {

void render_layout(gfx::Painter &, layout::LayoutBox const &);

namespace debug {
void render_layout_depth(gfx::Painter &, layout::LayoutBox const &);
} // namespace debug
} // namespace render

#endif
