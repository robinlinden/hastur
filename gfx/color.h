// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef GFX_COLOR_H_
#define GFX_COLOR_H_

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

    constexpr static Color from_rgba(std::int32_t rgba) {
        auto alpha = static_cast<std::uint8_t>(rgba & 0xFF);
        auto color = from_rgb((rgba & 0xFF'FF'FF'00) >> 8);
        color.a = alpha;
        return color;
    }

    std::uint8_t r, g, b, a{0xFF};

    [[nodiscard]] constexpr std::uint32_t as_rgba_u32() const { return r << 24 | g << 16 | b << 8 | a; }

    [[nodiscard]] constexpr bool operator==(Color const &) const = default;
};

} // namespace gfx

#endif
