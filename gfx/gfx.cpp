// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "gfx/gfx.h"

// MSVC gl.h doesn't include everything it uses.
#ifdef _MSC_VER
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // _MSC_VER

#include <GL/gl.h>

namespace gfx {

void OpenGLPainter::fill_rect(geom::Rect const &rect, Color color) {
    auto translated{rect.translated(translation_x, translation_y)};
    glColor3ub(color.r, color.g, color.b);
    glRecti(translated.x, translated.y, translated.x + translated.width, translated.y + translated.height);
}

} // namespace gfx
