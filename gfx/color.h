// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef GFX_COLOR_H_
#define GFX_COLOR_H_

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string_view>

namespace gfx {

struct Color {
    constexpr static Color from_rgb(std::uint32_t rgb) {
        return Color{
                .r = static_cast<std::uint8_t>((rgb & 0xFF0000) >> 16),
                .g = static_cast<std::uint8_t>((rgb & 0x00FF00) >> 8),
                .b = static_cast<std::uint8_t>(rgb & 0x0000FF),
        };
    }

    constexpr static Color from_rgba(std::uint32_t rgba) {
        auto alpha = static_cast<std::uint8_t>(rgba & 0xFF);
        auto color = from_rgb((rgba & 0xFF'FF'FF'00) >> 8);
        color.a = alpha;
        return color;
    }

    static Color from_hsl(float hue, float saturation, float light) { return from_hsla(hue, saturation, light, 1.f); }
    static Color from_hsla(float hue, float saturation, float light, float alpha) {
        // https://www.w3.org/TR/css-color-3/#hsl-color
        hue = std::fmod(hue, 360.f);
        if (hue < 0.f) {
            hue += 360.f;
        }

        saturation = std::clamp(saturation, 0.f, 1.f);
        light = std::clamp(light, 0.f, 1.f);
        alpha = std::clamp(alpha, 0.f, 1.f);

        auto hue_to_rgb = [&](int n) {
            float k = std::fmod(n + hue / 30.f, 12.f);
            float aa = saturation * std::min(light, 1.f - light);
            return light - aa * std::clamp(std::min(k - 3.f, 9.f - k), -1.f, 1.f);
        };

        return {
                .r = static_cast<std::uint8_t>(std::lround(255 * hue_to_rgb(0))),
                .g = static_cast<std::uint8_t>(std::lround(255 * hue_to_rgb(8))),
                .b = static_cast<std::uint8_t>(std::lround(255 * hue_to_rgb(4))),
                .a = static_cast<std::uint8_t>(std::lround(255 * alpha)),
        };
    }

    static std::optional<Color> from_css_name(std::string_view);

    std::uint8_t r, g, b, a{0xFF};

    [[nodiscard]] constexpr std::uint32_t as_rgba_u32() const { return r << 24 | g << 16 | b << 8 | a; }

    [[nodiscard]] constexpr bool operator==(Color const &) const = default;
};

} // namespace gfx

#endif
