// SPDX-FileCopyrightText: 2024-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "idna/uts46.h"

#include "idna/idna_data.h"
#include "idna/punycode.h"

#include "unicode/normalization.h"
#include "unicode/util.h"
#include "util/string.h"

#include <algorithm>
#include <cstddef>
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

// https://www.unicode.org/reports/tr46/#ToUnicode
std::optional<std::string> Uts46::to_unicode(std::string_view domain_name) {
    // 1. Map.
    auto processed = map(domain_name);
    if (!processed) {
        return std::nullopt;
    }

    // 2. Normalize.
    processed = unicode::Normalization::nfc(*processed);

    // 3. Break.
    auto labels = util::split(*processed, ".");

    // 4. Convert/Validate.
    std::string res;
    for (std::size_t i = 0; i < labels.size(); ++i) {
        auto &label = labels[i];
        if (label.starts_with("xn--")) {
            if (!std::ranges::all_of(label, &unicode::is_ascii)) {
                return std::nullopt;
            }

            auto decoded = Punycode::to_utf8(label.substr(4));
            if (!decoded || decoded->empty() || std::ranges::all_of(*decoded, &unicode::is_ascii)) {
                return std::nullopt;
            }

            // TODO(robinlinden): validate.

            res += *decoded;
            if (i + 1 < labels.size()) {
                res += ".";
            }

            continue;
        }

        // TODO(robinlinden): validate.

        res += label;
        if (i + 1 < labels.size()) {
            res += ".";
        }
    }

    return res;
}

} // namespace idna
