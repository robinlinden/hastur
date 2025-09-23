// SPDX-FileCopyrightText: 2024-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "unicode/normalization.h"

#include "unicode/unicode_data.h"
#include "unicode/util.h"

#include <algorithm>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace unicode {
namespace {

// NOLINTNEXTLINE(misc-no-recursion)
void decompose_to(std::ostream &os, char32_t code_point) {
    auto maybe_decomposition = std::ranges::lower_bound(
            generated::kDecompositions, code_point, {}, &decltype(generated::kDecompositions)::value_type::code_point);

    // This code point does not decompose.
    if (maybe_decomposition->code_point != code_point) {
        os << to_utf8(code_point);
        return;
    }

    // Recursively decompose the decomposition. This is needed as some code
    // points decompose into code points that also decompose.
    for (auto const decomposed : CodePointView{maybe_decomposition->decomposes_to}) {
        decompose_to(os, decomposed);
    }
}

} // namespace

std::string Normalization::decompose(std::string_view input) {
    std::stringstream ss;

    for (auto const code_point : CodePointView{input}) {
        decompose_to(ss, code_point);
    }

    return std::move(ss).str();
}

} // namespace unicode
