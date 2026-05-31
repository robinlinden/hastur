// SPDX-FileCopyrightText: 2024-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "unicode/normalization.h"

#include "unicode/canonical_combining_class_data.h"
#include "unicode/composition_data.h"
#include "unicode/decomposition_data.h"
#include "unicode/util.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace unicode {
namespace {

// NOLINTNEXTLINE(misc-no-recursion)
void decompose_to(std::ostream &os, char32_t code_point) {
    auto maybe_decomposition = std::ranges::lower_bound(
            generated::kDecompositions, code_point, {}, &decltype(generated::kDecompositions)::value_type::code_point);

    // This code point does not decompose.
    if (maybe_decomposition == generated::kDecompositions.end() || maybe_decomposition->code_point != code_point) {
        os << to_utf8(code_point);
        return;
    }

    // Recursively decompose the decomposition. This is needed as some code
    // points decompose into code points that also decompose.
    for (auto const decomposed : CodePointView{maybe_decomposition->decomposes_to}) {
        decompose_to(os, decomposed);
    }
}

std::uint8_t canonical_combining_class(char32_t code_point) {
    auto cls = std::ranges::lower_bound(generated::kCanonicalCombiningClasses,
            code_point,
            {},
            &decltype(generated::kCanonicalCombiningClasses)::value_type::first);

    if (cls == generated::kCanonicalCombiningClasses.end()) {
        return generated::kCanonicalCombiningClasses.back().second;
    }

    // TODO(robinlinden): Generate better mapping table.
    if (cls->first != code_point) {
        cls -= 1;
    }

    return cls->second;
}

} // namespace

// TODO(robinlinden): This is very inefficient. Make it less silly.
std::string Normalization::nfd(std::string_view input) {
    std::stringstream ss;

    for (auto const code_point : CodePointView{input}) {
        decompose_to(ss, code_point);
    }

    auto decomposed = std::exchange(ss, {}).str();
    std::vector<decltype(generated::kCanonicalCombiningClasses)::value_type> might_need_rearrangement;
    for (auto const code_point : CodePointView{decomposed}) {
        if (auto cls = canonical_combining_class(code_point); cls != 0) {
            might_need_rearrangement.emplace_back(code_point, cls);
        } else {
            std::ranges::stable_sort(
                    might_need_rearrangement, {}, &decltype(generated::kCanonicalCombiningClasses)::value_type::second);
            for (auto [c, _] : might_need_rearrangement) {
                decompose_to(ss, c);
            }

            decompose_to(ss, code_point);
            might_need_rearrangement.clear();
        }
    }

    std::ranges::stable_sort(
            might_need_rearrangement, {}, &decltype(generated::kCanonicalCombiningClasses)::value_type::second);
    for (auto [c, _] : might_need_rearrangement) {
        decompose_to(ss, c);
    }

    return std::move(ss).str();
}

// TODO(robinlinden): This is very inefficient. Make it less silly.
std::string Normalization::nfc(std::string_view input) {
    auto decomposed = nfd(input);
    std::stringstream ss;

    std::vector<char32_t> code_points;
    for (auto code_point : CodePointView{decomposed}) {
        code_points.push_back(code_point);
    }

    for (std::size_t i = 0; i < code_points.size(); ++i) {
        auto code_point = code_points[i];

        while (true) {
            // The last code point can't be composed w/ anything.
            if (i == code_points.size() - 1) {
                break;
            }

            auto maybe_composition = std::ranges::lower_bound(
                    generated::kCompositions, code_point, {}, &decltype(generated::kCompositions)::value_type::first);

            if (maybe_composition == generated::kCompositions.end() || maybe_composition->first != code_point) {
                break;
            }

            assert(i + 1 < code_points.size());
            auto next_code_point = code_points[i + 1];
            while (maybe_composition != generated::kCompositions.end() && maybe_composition->first == code_point) {
                if (maybe_composition->second == next_code_point) {
                    break;
                }

                ++maybe_composition;
            }

            if (maybe_composition->second != next_code_point) {
                break;
            }

            // We found a composition, so we skip the code point we ate, set the
            // current code point to the composed code point, and try to compose
            // again.
            code_point = maybe_composition->composed;
            i += 1;
        }

        ss << to_utf8(code_point);
    }

    return std::move(ss).str();
}

} // namespace unicode
