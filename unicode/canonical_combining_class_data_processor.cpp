// SPDX-FileCopyrightText: 2024-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "util/string.h"

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

    std::cout << R"(// SPDX-FileCopyrightText: 2024-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

// This file is generated. Do not touch it.

#ifndef UNICODE_CANONICAL_COMBINING_CLASS_DATA_H_
#define UNICODE_CANONICAL_COMBINING_CLASS_DATA_H_

#include <array>
#include <cstdint>
#include <utility>

namespace unicode::generated {

// List of each code point starting a new run of a specific canonical combining
// class. I.e. if code point 1 and 2 are of class 0, and 3 is of class 240,
// this list will be `[(1, 0), (3, 240)]`.
constexpr auto kCanonicalCombiningClasses = std::to_array<std::pair<char32_t, std::uint8_t>>({
)";

    // The classes are in the range 0-240, so using -1 as a sentinel is fine.
    int last_seen_class = -1;
    for (std::string line{}; std::getline(table, line);) {
        auto fields = util::split(line, ";");
        if (fields.size() != 15) {
            std::cerr << "Invalid row '" << line << "' in '" << argv[1] << "'.\n";
            return 1;
        }

        // Skip the entry if this class just continues the run.
        int cls{};
        auto const *field_start = fields[3].data();
        auto const *field_end = fields[3].data() + fields[3].length();
        if (auto fc_res = std::from_chars(field_start, field_end, cls);
                fc_res.ec != std::errc{} || fc_res.ptr != field_end) {
            std::cerr << "Unable to parse '" << fields[3] << "' as an int\n";
            return 1;
        }

        if (cls == last_seen_class) {
            continue;
        }

        last_seen_class = cls;
        std::int32_t code_point = code_point_from_hex(fields[0]);

        std::cout << "        {" << code_point << ", " << cls << "},\n";
    }

    std::cout << "});\n\n"
              << "} // namespace unicode::generated\n\n"
              << "#endif\n";
}
