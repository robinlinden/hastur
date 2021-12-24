// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef GFX_GFX_H_
#define GFX_GFX_H_

#include "geom/geom.h"

#include <cstdint>

namespace gfx {

struct Color {
    constexpr static Color from_rgb(std::int32_t rgb) {
        return Color{
                .r = static_cast<std::uint8_t>((rgb & 0xFF0000) >> 16),
                .g = static_cast<std::uint8_t>((rgb & 0x00FF00) >> 8),
                .b = static_cast<std::uint8_t>(rgb & 0x0000FF),
        };
    }

    std::uint8_t r, g, b, a{0xFF};

    [[nodiscard]] constexpr bool operator==(Color const &) const = default;
};

class IPainter {
public:
    virtual ~IPainter() = default;

    virtual void set_viewport_size(int width, int height) = 0;
    virtual void set_scale(int scale) = 0;
    virtual void add_translation(int dx, int dy) = 0;
    virtual void fill_rect(geom::Rect const &, Color) = 0;
};

class OpenGLPainter final : public IPainter {
public:
    OpenGLPainter();

    void set_viewport_size(int width, int height) override;
    constexpr void set_scale(int scale) override { scale_ = scale; }

    constexpr void add_translation(int dx, int dy) override {
        translation_x += dx;
        translation_y += dy;
    }

    void fill_rect(geom::Rect const &, Color) override;

private:
    int translation_x{};
    int translation_y{};
    int scale_{1};
};

} // namespace gfx

#endif
