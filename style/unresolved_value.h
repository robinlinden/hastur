// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef STYLE_UNRESOLVED_VALUE_H_
#define STYLE_UNRESOLVED_VALUE_H_

#include <optional>
#include <source_location>
#include <string_view>

namespace style {

struct ResolutionInfo {
    int root_font_size{};
    int viewport_width{};
    int viewport_height{};
};

struct UnresolvedValue {
    std::string_view raw{};
    [[nodiscard]] bool operator==(UnresolvedValue const &) const = default;

    constexpr bool is_auto() const { return raw == "auto"; }
    constexpr bool is_none() const { return raw == "none"; }
    int resolve(int font_size,
            ResolutionInfo,
            std::optional<int> percent_relative_to = std::nullopt,
            std::source_location const &caller = std::source_location::current()) const;
    std::optional<int> try_resolve(int font_size,
            ResolutionInfo,
            std::optional<int> percent_relative_to = std::nullopt,
            std::source_location const &caller = std::source_location::current()) const;
};

} // namespace style

#endif
