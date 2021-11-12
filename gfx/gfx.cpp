// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "gfx/gfx.h"

#include <GL/glew.h>

namespace gfx {

OpenGLPainter::OpenGLPainter() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void OpenGLPainter::fill_rect(geom::Rect const &rect, Color color) {
    auto translated{rect.translated(translation_x, translation_y)};
    glColor4ub(color.r, color.g, color.b, color.a);
    glRecti(translated.x, translated.y, translated.x + translated.width, translated.y + translated.height);
}

} // namespace gfx
