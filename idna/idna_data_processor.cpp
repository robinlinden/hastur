// SPDX-FileCopyrightText: 2023-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "util/string.h"

#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <fstream>
#include <iostream>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>
#include <vector>

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

template<typename T>
T parse_from_string(std::string_view s) {
    if (s.empty()) {
        return T{};
    }

    std::vector<std::uint32_t> maps_to;
    for (auto part : std::ranges::views::split(s, ' ')) {
        maps_to.push_back(code_point_from_hex(std::string_view{part}));
    }

    return T{std::move(maps_to)};
}

struct Disallowed {
    constexpr bool operator==(Disallowed const &) const = default;
};

struct Ignored {
    constexpr bool operator==(Ignored const &) const = default;
};

struct Mapped {
    std::vector<std::uint32_t> maps_to;
    constexpr bool operator==(Mapped const &) const = default;

    static Mapped from_string(std::string_view s) { return parse_from_string<Mapped>(s); }
};

struct Deviation {
    std::vector<std::uint32_t> maps_to;
    constexpr bool operator==(Deviation const &) const = default;

    static Deviation from_string(std::string_view s) { return parse_from_string<Deviation>(s); }
};

struct Valid {
    constexpr bool operator==(Valid const &) const = default;
};

struct ValidNv8 {
    constexpr bool operator==(ValidNv8 const &) const = default;
};

struct ValidXv8 {
    constexpr bool operator==(ValidXv8 const &) const = default;
};

using Mapping = std::variant<Disallowed, Ignored, Mapped, Deviation, Valid, ValidNv8, ValidXv8>;

template<typename T>
bool last_mapping_was(auto const &mappings, std::optional<T> mapping = std::nullopt) {
    if (mappings.empty()) {
        return false;
    }

    if (!mapping) {
        return std::holds_alternative<T>(mappings.back().second);
    }

    auto const *last = std::get_if<T>(&mappings.back().second);
    return last != nullptr && *last == *mapping;
};

struct Idna {
    // List of each code point starting a new mapping. I.e. if code point 1
    // and 2 are disallowed, and 3 is valid, this list will be
    // `[(1, Disallowed), (3, Valid)]`.
    std::vector<std::pair<std::uint32_t, Mapping>> mappings;

    // https://www.unicode.org/reports/tr46/#Table_Data_File_Fields
    static Idna from_table(std::istream &is) {
        Idna idna;

        for (std::string row; std::getline(is, row);) {
            // Drop the trailing comment about what code point this is.
            row = util::split_once(row, '#').first;

            auto cols = util::split(row, ";");
            for (auto &col : cols) {
                col = util::trim(col);
            }

            // Some rows are blank or just a comment.
            if (cols.size() <= 1) {
                continue;
            }

            std::uint32_t code_point = [](std::string_view s) {
                if (auto it = s.find(".."); it != std::string_view::npos) {
                    s.remove_prefix(it + 2);
                }

                return code_point_from_hex(s);
            }(cols[0]);

            auto status = cols[1];
            if (status == "disallowed") {
                if (last_mapping_was<Disallowed>(idna.mappings)) {
                    idna.mappings.back().first = code_point;
                    continue;
                }
                idna.mappings.emplace_back(code_point, Disallowed{});
            } else if (status == "ignored") {
                if (last_mapping_was<Ignored>(idna.mappings)) {
                    idna.mappings.back().first = code_point;
                    continue;
                }
                idna.mappings.emplace_back(code_point, Ignored{});
            } else if (status == "mapped") {
                if (last_mapping_was<Mapped>(idna.mappings, Mapped::from_string(cols[2]))) {
                    idna.mappings.back().first = code_point;
                    continue;
                }
                idna.mappings.emplace_back(code_point, Mapped::from_string(cols[2]));
            } else if (status == "deviation") {
                if (last_mapping_was<Deviation>(idna.mappings, Deviation::from_string(cols[2]))) {
                    idna.mappings.back().first = code_point;
                    continue;
                }
                idna.mappings.emplace_back(code_point, Deviation::from_string(cols[2]));
            } else if (status == "valid" && cols.size() == 2) {
                if (last_mapping_was<Valid>(idna.mappings)) {
                    idna.mappings.back().first = code_point;
                    continue;
                }
                idna.mappings.emplace_back(code_point, Valid{});
            } else if (status == "valid" && cols.size() == 4 && cols[3] == "NV8") {
                if (last_mapping_was<ValidNv8>(idna.mappings)) {
                    idna.mappings.back().first = code_point;
                    continue;
                }
                idna.mappings.emplace_back(code_point, ValidNv8{});
            } else if (status == "valid" && cols.size() == 4 && cols[3] == "XV8") {
                if (last_mapping_was<ValidXv8>(idna.mappings)) {
                    idna.mappings.back().first = code_point;
                    continue;
                }
                idna.mappings.emplace_back(code_point, ValidXv8{});
            } else {
                std::cerr << "Unable to parse data: " << cols[0] << "\n";
                std::abort();
            }
        }

        return idna;
    }
};

// The version of MSVC we target doesn't yet support \u{12abc}, so we always
// write the full 8-character escapes.
std::string to_cxx_variant(Mapping const &a) {
    struct Stringifier {
        std::string operator()(Disallowed const &) const { return "Disallowed{}"; }
        std::string operator()(Ignored const &) const { return "Ignored{}"; }

        std::string operator()(Mapped const &a) const {
            std::string result;
            for (auto c : a.maps_to) {
                result += "\\U" + std::format("{:08X}", c);
            }

            return "Mapped{\"" + result + "\"}";
        }

        std::string operator()(Deviation const &a) const {
            std::string result;
            for (auto c : a.maps_to) {
                result += "\\U" + std::format("{:08X}", c);
            }

            return "Deviation{\"" + result + "\"}";
        }

        std::string operator()(Valid const &) const { return "Valid{}"; }
        std::string operator()(ValidNv8 const &) const { return "ValidNv8{}"; }
        std::string operator()(ValidXv8 const &) const { return "ValidXv8{}"; }
    };

    return std::visit(Stringifier{}, a);
}

} // namespace

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <IdnaMappingTable.txt>\n";
        return 1;
    }

    auto table = std::ifstream{argv[1]};
    if (!table) {
        std::cerr << "Unable to open file: " << argv[1] << '\n';
        return 1;
    }

    auto idna = Idna::from_table(table);

    std::cout << R"(// SPDX-FileCopyrightText: 2023-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

// This file is generated. Do not touch it.

#ifndef IDNA_IDNA_DATA_H_
#define IDNA_IDNA_DATA_H_

#include <array>
#include <string_view>
#include <variant>
#include <utility>

namespace idna::uts46 {

struct Disallowed {
    constexpr bool operator==(Disallowed const &) const = default;
};

struct Ignored {
    constexpr bool operator==(Ignored const &) const = default;
};

struct Mapped {
    std::string_view maps_to;
    constexpr bool operator==(Mapped const &) const = default;
};

struct Deviation {
    std::string_view maps_to;
    constexpr bool operator==(Deviation const &) const = default;
};

struct Valid {
    constexpr bool operator==(Valid const &) const = default;
};

struct ValidNv8 {
    constexpr bool operator==(ValidNv8 const &) const = default;
};

struct ValidXv8 {
    constexpr bool operator==(ValidXv8 const &) const = default;
};

using Mapping = std::variant<
        Disallowed,
        Ignored,
        Mapped,
        Deviation,
        Valid,
        ValidNv8,
        ValidXv8>;

constexpr std::array<std::pair<char32_t, Mapping>, )"
              << idna.mappings.size() << R"(> kMappings{{
)";

    for (auto const &[code_point, mapping] : idna.mappings) {
        std::cout << "    {" << code_point << ", Mapping{" << to_cxx_variant(mapping) << "}},\n";
    }

    std::cout << R"(}};

} // namespace idna::uts46

#endif
)";
}
