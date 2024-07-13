// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "style/unresolved_value.h"

#include "util/from_chars.h"

#include <spdlog/spdlog.h>

#include <iterator>
#include <optional>
#include <string_view>
#include <system_error>

namespace style {

int UnresolvedValue::resolve(int font_size, int root_font_size, std::optional<int> percent_relative_to) const {
    return to_px(raw, font_size, root_font_size, percent_relative_to);
}

std::optional<int> UnresolvedValue::try_resolve(
        int font_size, int root_font_size, std::optional<int> percent_relative_to) const {
    return try_to_px(raw, font_size, root_font_size, percent_relative_to);
}

std::optional<int> try_to_px(std::string_view property,
        int const font_size,
        int const root_font_size,
        std::optional<int> parent_property_value) {
    // Special case for 0 since it won't ever have a unit that needs to be handled.
    if (property == "0") {
        return 0;
    }

    float res{};
    auto parse_result = util::from_chars(property.data(), property.data() + property.size(), res);
    if (parse_result.ec != std::errc{}) {
        spdlog::warn("Unable to parse property '{}' in to_px", property);
        return std::nullopt;
    }

    auto const parsed_length = std::distance(property.data(), parse_result.ptr);
    auto const unit = property.substr(parsed_length);

    if (unit == "%") {
        if (!parent_property_value.has_value()) {
            spdlog::warn("Missing parent-value for property w/ '%' unit");
            return std::nullopt;
        }

        return static_cast<int>(res / 100.f * (*parent_property_value));
    }

    if (unit == "px") {
        return static_cast<int>(res);
    }

    if (unit == "em") {
        res *= static_cast<float>(font_size);
        return static_cast<int>(res);
    }

    if (unit == "rem") {
        res *= static_cast<float>(root_font_size);
        return static_cast<int>(res);
    }

    // https://www.w3.org/TR/css3-values/#ex
    // https://www.w3.org/TR/css3-values/#ch
    if (unit == "ex" || unit == "ch") {
        // Technically, these are the height of an 'x' or '0' glyph
        // respectively, but we're allowed to approximate it as 50% of the em
        // value.
        static constexpr float kExToEmRatio = 0.5f;
        return static_cast<int>(res * kExToEmRatio * font_size);
    }

    spdlog::warn("Bad property '{}' w/ unit '{}' in to_px", property, unit);
    return std::nullopt;
}

} // namespace style
