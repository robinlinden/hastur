// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "type/last_resort_font.h"

#include <cstddef>
#include <span>

namespace type {
namespace {
#include "type/last_resort_font_data.h"
} // namespace

std::span<std::byte const> last_resort_font_ttf_data() {
    return std::span<std::byte const>{reinterpret_cast<std::byte const *>(last_resort_font), last_resort_font_len};
}

} // namespace type
