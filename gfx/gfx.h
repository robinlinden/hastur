// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef GFX_GFX_H_
#define GFX_GFX_H_

#include "geom/geom.h"

#include <cstdint>

namespace gfx {

struct Color {
    std::uint8_t r, g, b;
};

class IPainter {
public:
    virtual ~IPainter() = default;

    virtual void fill_rect(geom::Rect const &, Color) = 0;
};

class OpenGLPainter final : public IPainter {
public:
    void fill_rect(geom::Rect const &, Color) override;
};

} // namespace gfx

#endif
