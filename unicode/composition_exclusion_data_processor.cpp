// SPDX-FileCopyrightText: 2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "unicode/canonical_combining_class_data.h"

#include "util/string.h"

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>

namespace {

std::uint8_t canonical_combining_class(char32_t code_point) {
    auto cls = std::ranges::lower_bound(unicode::generated::kCanonicalCombiningClasses,
            code_point,
            {},
            &decltype(unicode::generated::kCanonicalCombiningClasses)::value_type::first);

    // TODO(robinlinden): Generate better mapping table.
    if (cls->first != code_point) {
        cls -= 1;
    }

    return cls->second;
}

std::uint32_t code_point_from_hex(std::string_view s) {
    std::uint32_t cp{};
    auto res = std::from_chars(s.data(), s.data() + s.size(), cp, 16);
    if (res.ec != std::errc{} || res.ptr != s.data() + s.size()) {
        std::cerr << "Unable to parse code point: " << s << '\n';
        std::abort();
    }

    return cp;
}

} // namespace

// https://www.unicode.org/reports/tr15/#Primary_Exclusion_List_Table
int main(int argc, char **argv) {
    if (argc != 3) {
        auto const *bin_name = argv[0] != nullptr ? argv[0] : "<bin>";
        std::cerr << "Usage: " << bin_name << " <CompositionExclusions.txt> <UnicodeData.txt>\n";
        return 1;
    }

    std::ifstream ce{argv[1]};
    if (!ce) {
        std::cerr << "Unable to open " << argv[1] << " for reading\n";
        return 1;
    }

    std::ifstream ud{argv[2]};
    if (!ud) {
        std::cerr << "Unable to open " << argv[2] << " for reading\n";
        return 1;
    }

    std::cout << R"(// SPDX-FileCopyrightText: 2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

// This file is generated. Do not touch it.

#ifndef UNICODE_COMPOSITION_EXCLUSION_DATA_H_
#define UNICODE_COMPOSITION_EXCLUSION_DATA_H_

#include <array>

namespace unicode::generated {

constexpr auto kCompositionExclusions = std::to_array<char32_t>({
)";

    for (std::string line{}; std::getline(ce, line);) {
        if (line.empty() || !util::is_hex_digit(line.front())) {
            continue;
        }

        auto [code_point, _] = util::split_once(line, ' ');

        std::cout << "        0x" << code_point << ",\n";
    }

    for (std::string line{}; std::getline(ud, line);) {
        auto fields = util::split(line, ";");
        if (fields.size() != 15) {
            std::cerr << "Invalid row '" << line << "' in '" << argv[1] << "'.\n";
            return 1;
        }

        // Filter out non-canonical decompositions.
        auto decomposition_field = fields[5];
        if (decomposition_field.empty() || decomposition_field[0] == '<') {
            continue;
        }

        bool has_single_char_decomposition = !decomposition_field.contains(' ');
        if (!has_single_char_decomposition) {
            auto combining_class_field = fields[3];

            auto [first_decomposition_character, _] = util::split_once(decomposition_field, ' ');
            std::uint32_t code_point = code_point_from_hex(first_decomposition_character);
            auto decomp_combining_class = canonical_combining_class(code_point);

            if (combining_class_field == "0" && decomp_combining_class == 0) {
                continue;
            }
        }

        std::cout << "        0x" << fields[0] << ",\n";
    }

    std::cout << "});\n\n"
              << "} // namespace unicode::generated\n\n"
              << "#endif\n";
}
