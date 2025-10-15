// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "type/fallback_font.h"

#include "type/fallback_font_data.h"

#include <cstddef>
#include <span>

namespace type {

std::span<std::byte const> fallback_font_ttf_data() {
    return std::span<std::byte const>{reinterpret_cast<std::byte const *>(kFallbackFont.data()), kFallbackFont.size()};
}

} // namespace type
