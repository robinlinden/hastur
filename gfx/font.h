// SPDX-FileCopyrightText: 2022-2024 Robin Lindén <dev@robinlinden.eu>
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

struct FontStyle {
    bool bold{false};
    bool italic{false};
    bool strikethrough{false};
    bool underlined{false};

    [[nodiscard]] constexpr bool operator==(FontStyle const &) const = default;
};

} // namespace gfx

#endif
