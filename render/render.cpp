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
namespace {

void render_layout_impl(layout::LayoutBox const &layout, int depth) {
    auto const &dimensions = layout.dimensions.content;
    float color = 1.f / depth;
    glColor3f(color, color, color);
    glRectf(dimensions.x, dimensions.y, dimensions.x + dimensions.width, dimensions.y + dimensions.height);
    for (auto const &child : layout.children) {
        render_layout_impl(child, depth + 1);
    }
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

void render_layout(layout::LayoutBox const &layout) {
    render_layout_impl(layout, 1);
}

} // namespace render
