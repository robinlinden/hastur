// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
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

    std::uint8_t r, g, b, a{0xFF};

    [[nodiscard]] constexpr bool operator==(Color const &) const = default;
};

} // namespace gfx

#endif
