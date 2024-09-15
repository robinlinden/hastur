// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "style/unresolved_value.h"

#include "util/from_chars.h"

#include <spdlog/spdlog.h>

#include <iterator>
#include <optional>
#include <source_location>
#include <string_view>
#include <system_error>

namespace style {

int UnresolvedValue::resolve(int font_size,
        ResolutionInfo context,
        std::optional<int> percent_relative_to,
        std::source_location const &caller) const {
    return try_resolve(font_size, context, percent_relative_to, caller).value_or(0);
}

std::optional<int> UnresolvedValue::try_resolve(int font_size,
        ResolutionInfo context,
        std::optional<int> percent_relative_to,
        std::source_location const &caller) const {
    // Special case for 0 since it won't ever have a unit that needs to be handled.
    if (raw == "0") {
        return 0;
    }

    float res{};
    auto parse_result = util::from_chars(raw.data(), raw.data() + raw.size(), res);
    if (parse_result.ec != std::errc{}) {
        spdlog::warn("{}({}:{}): Unable to parse property '{}' in to_px",
                caller.file_name(),
                caller.line(),
                caller.column(),
                raw);
        return std::nullopt;
    }

    auto const parsed_length = std::distance(raw.data(), parse_result.ptr);
    auto const unit = raw.substr(parsed_length);

    if (unit == "%") {
        if (!percent_relative_to.has_value()) {
            spdlog::warn("{}({}:{}): Missing parent-value for property w/ '%' unit",
                    caller.file_name(),
                    caller.line(),
                    caller.column());
            return std::nullopt;
        }

        return static_cast<int>(res / 100.f * (*percent_relative_to));
    }

    if (unit == "px") {
        return static_cast<int>(res);
    }

    if (unit == "em") {
        res *= static_cast<float>(font_size);
        return static_cast<int>(res);
    }

    if (unit == "rem") {
        res *= static_cast<float>(context.root_font_size);
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

    // https://www.w3.org/TR/css3-values/#vw
    if (unit == "vw") {
        res *= static_cast<float>(context.viewport_width) / 100;
        return static_cast<int>(res);
    }

    spdlog::warn("{}({}:{}): Bad property '{}' w/ unit '{}' in to_px",
            caller.file_name(),
            caller.line(),
            caller.column(),
            raw,
            unit);
    return std::nullopt;
}

} // namespace style
