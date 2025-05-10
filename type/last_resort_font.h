// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef TYPE_LAST_RESORT_FONT_H_
#define TYPE_LAST_RESORT_FONT_H_

#include <cstddef>
#include <span>

namespace type {

std::span<std::byte const> last_resort_font_ttf_data();

} // namespace type

#endif
