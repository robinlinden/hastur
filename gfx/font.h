// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef GFX_FONT_H_
#define GFX_FONT_H_

#include <string_view>

namespace gfx {

struct Font {
    std::string_view font;
};

struct FontSize {
    int px{10};
};

enum class FontStyle {
    Normal,
    Italic,
};

} // namespace gfx

#endif
