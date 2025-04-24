// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "type/fallback_font.h"

#include <cstddef>
#include <span>

namespace type {
namespace {
#include "type/fallback_font_data.h"
} // namespace

std::span<std::byte const> fallback_font_ttf_data() {
    return std::span<std::byte const>{reinterpret_cast<std::byte const *>(fallback_font), fallback_font_len};
}

} // namespace type
