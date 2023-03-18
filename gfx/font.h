// SPDX-FileCopyrightText: 2022-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef GFX_FONT_H_
#define GFX_FONT_H_

#include <string_view>
#include <utility>

namespace gfx {

struct Font {
    std::string_view font;
};

struct FontSize {
    int px{10};
};

enum class FontStyle {
    Normal = 0,
    Italic = 1 << 0,
};

constexpr FontStyle operator|(FontStyle lhs, FontStyle rhs) {
    return static_cast<FontStyle>(std::to_underlying(lhs) | std::to_underlying(rhs));
}

constexpr FontStyle &operator|=(FontStyle &lhs, FontStyle rhs) {
    lhs = lhs | rhs;
    return lhs;
}

constexpr FontStyle operator&(FontStyle lhs, FontStyle rhs) {
    return static_cast<FontStyle>(std::to_underlying(lhs) & std::to_underlying(rhs));
}

constexpr FontStyle &operator&=(FontStyle &lhs, FontStyle rhs) {
    lhs = lhs & rhs;
    return lhs;
}

constexpr FontStyle operator^(FontStyle lhs, FontStyle rhs) {
    return static_cast<FontStyle>(std::to_underlying(lhs) ^ std::to_underlying(rhs));
}

constexpr FontStyle &operator^=(FontStyle &lhs, FontStyle rhs) {
    lhs = lhs ^ rhs;
    return lhs;
}

constexpr FontStyle operator~(FontStyle v) {
    return static_cast<FontStyle>(~std::to_underlying(v));
}

} // namespace gfx

#endif
