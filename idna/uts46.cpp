// SPDX-FileCopyrightText: 2024-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "idna/uts46.h"

#include "idna/idna_data.h"

#include "unicode/util.h"

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace idna {

std::optional<std::string> Uts46::map(std::string_view input) {
    std::string result;
    // input.size is just an estimate, but probably good enough for now.
    result.reserve(input.size());

    for (auto const code_point : unicode::CodePointView{input}) {
        auto mapping = std::ranges::lower_bound(
                uts46::kMappings, code_point, {}, &decltype(uts46::kMappings)::value_type::first);

        auto const &entry = mapping->second;
        if (std::holds_alternative<uts46::Ignored>(entry)) {
            continue;
        }

        if (std::holds_alternative<uts46::Disallowed>(entry)) {
            return std::nullopt;
        }

        if (auto const *mapped = std::get_if<uts46::Mapped>(&entry)) {
            result += mapped->maps_to;
            continue;
        }

        // These would be mapped in transitional processing, but we don't support that.
        if (std::holds_alternative<uts46::Deviation>(entry)) {
            result += unicode::to_utf8(code_point);
            continue;
        }

        if (std::holds_alternative<uts46::Valid>(entry) //
                || std::holds_alternative<uts46::ValidNv8>(entry) //
                || std::holds_alternative<uts46::ValidXv8>(entry)) {
            result += unicode::to_utf8(code_point);
            continue;
        }
    }

    return result;
}

} // namespace idna
