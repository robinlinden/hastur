// SPDX-FileCopyrightText: 2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "util/string.h"

#include <charconv>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <system_error>

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

    std::cout << R"(// SPDX-FileCopyrightText: 2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

// This file is generated. Do not touch it.

#ifndef UNICODE_UNICODE_DATA_H_
#define UNICODE_UNICODE_DATA_H_
// clang-format off

#include <array>
#include <string_view>

namespace unicode::generated {

struct Decomposition {
    char32_t code_point{};
    std::string_view decomposes_to;
    constexpr bool operator==(Decomposition const &) const = default;
};

constexpr auto kDecompositions = std::to_array<Decomposition>({
)";

    for (std::string line{}; std::getline(table, line);) {
        // Filter out lines not containing decomposition info.
        auto fields = util::split(line, ";");
        if (fields.size() < 6) {
            continue;
        }

        // Filter out non-canonical decompositions.
        auto decomposition_field = fields[5];
        if (decomposition_field.empty() || decomposition_field[0] == '<') {
            continue;
        }

        std::cout << "        {0x" << fields[0] << ", \"";

        auto decompositions = util::split(decomposition_field, " ");
        for (auto const &decomp : decompositions) {
            std::uint32_t code_point{};
            auto res = std::from_chars(decomp.data(), decomp.data() + decomp.size(), code_point, 16);
            if (res.ec != std::errc{} || res.ptr != decomp.data() + decomp.size()) {
                std::cerr << "Invalid code point: " << decomp << '\n';
                return 1;
            }

            std::cout << "\\U" << std::hex << std::setw(8) << std::setfill('0') << code_point;
        }

        std::cout << "\"},\n";
    }

    std::cout << "});\n\n"
              << "} // namespace unicode::generated\n\n"
              << "// clang-format on\n"
              << "#endif\n";
}
