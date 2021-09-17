// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef RENDER_RENDER_H_
#define RENDER_RENDER_H_

#include "layout/layout.h"

namespace render {

void render_setup(int width, int height);
void render_layout(layout::LayoutBox const &layout);

} // namespace render

#endif
