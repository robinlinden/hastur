// SPDX-FileCopyrightText: 2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "idna/unicode.h"

#include "idna/unicode_data.h"

#include "util/unicode.h"

// NOLINTNEXTLINE(misc-include-cleaner): This is used for std::ranges::lower_bound.
#include <algorithm>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace idna {
namespace {

void decompose_to(std::ostream &os, char32_t code_point) {
    // * clang-tidy thinks std::ranges::lower_bound is provided by
    //   <bits/ranges_algo.h> when it's actually provided by <algorithm>.
    // * clang-tidy says this is pointer-ish, but msvc disagrees.
    // NOLINTNEXTLINE(misc-include-cleaner,readability-qualified-auto)
    auto maybe_decomposition = std::ranges::lower_bound(
            unicode::kDecompositions, code_point, {}, &decltype(unicode::kDecompositions)::value_type::code_point);

    // This code point does not decompose.
    if (maybe_decomposition->code_point != code_point) {
        os << util::unicode_to_utf8(code_point);
        return;
    }

    // Recursively decompose the decomposition. This is needed as some code
    // points decompose into code points that also decompose.
    for (auto const decomposed : util::CodePointView{maybe_decomposition->decomposes_to}) {
        decompose_to(os, decomposed);
    }
}

} // namespace

std::string Unicode::decompose(std::string_view input) {
    std::stringstream ss{};

    for (auto const code_point : util::CodePointView{input}) {
        decompose_to(ss, code_point);
    }

    return std::move(ss).str();
}

} // namespace idna
