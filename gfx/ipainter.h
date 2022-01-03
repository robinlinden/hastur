// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef GFX_IPAINTER_H_
#define GFX_IPAINTER_H_

#include "geom/geom.h"
#include "gfx/color.h"

namespace gfx {

class IPainter {
public:
    virtual ~IPainter() = default;

    virtual void set_viewport_size(int width, int height) = 0;
    virtual void set_scale(int scale) = 0;
    virtual void add_translation(int dx, int dy) = 0;
    virtual void fill_rect(geom::Rect const &, Color) = 0;
};

} // namespace gfx

#endif
