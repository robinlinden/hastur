// SPDX-FileCopyrightText: 2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "idna/uts46.h"

#include "idna/idna_data.h"

#include "util/unicode.h"

// NOLINTNEXTLINE(misc-include-cleaner): This is used for std::ranges::lower_bound.
#include <algorithm>
#include <cassert>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace idna {

std::optional<std::string> Uts46::map(std::string_view input) {
    std::string result{};
    // input.size is just an estimate, but probably good enough for now.
    result.reserve(input.size());

    for (auto const code_point : util::CodePointView{input}) {
        // * clang-tidy thinks std::ranges::lower_bound is provided by
        //   <bits/ranges_algo.h> when it's actually provided by <algorithm>.
        // * clang-tidy says this is pointer-ish, but msvc disagrees.
        // NOLINTNEXTLINE(misc-include-cleaner,readability-qualified-auto)
        auto mapping = std::ranges::lower_bound(
                uts46::kMappings, code_point, {}, &decltype(uts46::kMappings)::value_type::first);

        // TODO(robinlinden): Generate better mapping table.
        if (mapping->first != code_point) {
            mapping -= 1;
        }

        auto const &entry = mapping->second;
        if (std::holds_alternative<uts46::Ignored>(entry)) {
            continue;
        }

        if (std::holds_alternative<uts46::Disallowed>(entry)) {
            return std::nullopt;
        }

        // tr46 strongly recommends using the std3 rules, so no opt-out for
        // this.
        if (std::holds_alternative<uts46::DisallowedStd3Valid>(entry)) {
            return std::nullopt;
        }

        if (std::holds_alternative<uts46::DisallowedStd3Mapped>(entry)) {
            return std::nullopt;
        }

        if (auto const *mapped = std::get_if<uts46::Mapped>(&entry)) {
            result += mapped->maps_to;
            continue;
        }

        // These would be mapped in transitional processing, but we don't support that.
        if (std::holds_alternative<uts46::Deviation>(entry)) {
            result += util::unicode_to_utf8(code_point);
            continue;
        }

        if (std::holds_alternative<uts46::Valid>(entry) //
                || std::holds_alternative<uts46::ValidNv8>(entry) //
                || std::holds_alternative<uts46::ValidXv8>(entry)) {
            result += util::unicode_to_utf8(code_point);
            continue;
        }
    }

    return result;
}

} // namespace idna
