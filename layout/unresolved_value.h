// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef LAYOUT_UNRESOLVED_VALUE_H_
#define LAYOUT_UNRESOLVED_VALUE_H_

#include <optional>
#include <string_view>

namespace layout {

struct UnresolvedValue {
    std::string_view raw{};
    [[nodiscard]] bool operator==(UnresolvedValue const &) const = default;

    constexpr bool is_auto() const { return raw == "auto"; }
    int resolve(int font_size, int root_font_size, std::optional<int> percent_relative_to = std::nullopt) const;
};

} // namespace layout

#endif
