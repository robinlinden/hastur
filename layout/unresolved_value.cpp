// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "layout/unresolved_value.h"

#include "layout/layout_box.h"

#include <optional>

namespace layout {

int UnresolvedValue::resolve(int font_size, int root_font_size, std::optional<int> percent_relative_to) const {
    return to_px(raw, font_size, root_font_size, percent_relative_to);
}

std::optional<int> UnresolvedValue::try_resolve(
        int font_size, int root_font_size, std::optional<int> percent_relative_to) const {
    return try_to_px(raw, font_size, root_font_size, percent_relative_to);
}

} // namespace layout
