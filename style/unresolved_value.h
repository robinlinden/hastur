// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef STYLE_UNRESOLVED_VALUE_H_
#define STYLE_UNRESOLVED_VALUE_H_

#include <optional>
#include <string_view>

namespace style {

struct UnresolvedValue {
    std::string_view raw{};
    [[nodiscard]] bool operator==(UnresolvedValue const &) const = default;

    constexpr bool is_auto() const { return raw == "auto"; }
    constexpr bool is_none() const { return raw == "none"; }
    int resolve(int font_size, int root_font_size, std::optional<int> percent_relative_to = std::nullopt) const;
    std::optional<int> try_resolve(
            int font_size, int root_font_size, std::optional<int> percent_relative_to = std::nullopt) const;
};

// TODO(robinlinden): This should be internal.
std::optional<int> try_to_px(std::string_view property,
        int font_size,
        int root_font_size,
        std::optional<int> parent_property_value = std::nullopt);

inline int to_px(std::string_view property,
        int font_size,
        int root_font_size,
        std::optional<int> parent_property_value = std::nullopt) {
    return try_to_px(property, font_size, root_font_size, parent_property_value).value_or(0);
}

} // namespace style

#endif
