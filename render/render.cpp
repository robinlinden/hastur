// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "render/render.h"

// MSVC gl.h doesn't include everything it uses.
#ifdef _MSC_VER
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // _MSC_VER

#include <GL/gl.h>

namespace render {

void render_setup(int width, int height) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1.0, 1.0);
    glViewport(0, 0, width, height);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

namespace debug {
namespace {
void render_layout_depth_impl(gfx::IPainter &painter, layout::LayoutBox const &layout, int depth) {
    auto color = static_cast<std::uint8_t>(255 / depth);
    painter.fill_rect(layout.dimensions.content, {color, color, color});
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
