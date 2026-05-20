// SPDX-FileCopyrightText: 2024-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "unicode/composition_exclusion_data.h"

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

// https://www.unicode.org/reports/tr44/#UnicodeData.txt
int main(int argc, char **argv) {
    if (argc != 2) {
        auto const *bin_name = argv[0] != nullptr ? argv[0] : "<bin>";
        std::cerr << "Usage: " << bin_name << " <UnicodeData.txt>\n";
        return 1;
    }

    std::ifstream table{argv[1]};
    if (!table) {
        std::cerr << "Unable to open " << argv[1] << " for reading\n";
        return 1;
    }

    std::cout << R"(// SPDX-FileCopyrightText: 2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

// This file is generated. Do not touch it.

#ifndef UNICODE_COMPOSITION_DATA_H_
#define UNICODE_COMPOSITION_DATA_H_

#include <array>

namespace unicode::generated {

struct Composition {
    char32_t first{};
    char32_t second{};
    char32_t composed{};
    constexpr bool operator==(Composition const &) const = default;
};

constexpr auto kCompositions = std::to_array<Composition>({
)";

    for (std::string line{}; std::getline(table, line);) {
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

        // Filter out non-primary decompositions.
        auto code_point = code_point_from_hex(fields[0]);
        if (std::ranges::contains(unicode::generated::kCompositionExclusions, code_point)) {
            continue;
        }

        auto decompositions = util::split(decomposition_field, " ");
        if (decompositions.size() != 2) {
            std::cerr << "Invalid decomposition for code point " << fields[0] << ": " << decomposition_field << '\n';
            return 1;
        }

        std::cout << "        {0x" << decompositions[0] << ", 0x" << decompositions[1] << ", 0x" << fields[0] << "},\n";
    }

    std::cout << "});\n\n"
              << "} // namespace unicode::generated\n\n"
              << "#endif\n";
}
