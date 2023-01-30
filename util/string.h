// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2021 Mikael Larsson <c.mikael.larsson@gmail.com>
// SPDX-FileCopyrightText: 2022-2023 David Zero <zero-one@zer0-one.net>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef UTIL_STRING_H_
#define UTIL_STRING_H_

#include <fmt/core.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <iterator>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace util {

constexpr bool is_c0(char c) {
    return c >= 0x00 && c <= 0x1f;
}

constexpr bool is_c0_or_space(char c) {
    return is_c0(c) || c == ' ';
}

constexpr bool is_tab_or_newline(char c) {
    return c == '\t' || c == '\n' || c == '\r';
}

constexpr bool is_upper_alpha(char c) {
    return c >= 'A' && c <= 'Z';
}

constexpr bool is_lower_alpha(char c) {
    return c >= 'a' && c <= 'z';
}

constexpr bool is_alpha(char c) {
    return is_upper_alpha(c) || is_lower_alpha(c);
}

constexpr bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

constexpr bool is_alphanumeric(char c) {
    return is_digit(c) || is_alpha(c);
}

constexpr bool is_upper_hex_digit(char c) {
    return is_digit(c) || (c >= 'A' && c <= 'F');
}

constexpr bool is_lower_hex_digit(char c) {
    return is_digit(c) || (c >= 'a' && c <= 'f');
}

constexpr bool is_hex_digit(char c) {
    return is_upper_hex_digit(c) || is_lower_hex_digit(c);
}

constexpr char lowercased(char c) {
    if (!is_upper_alpha(c)) {
        return c;
    }

    return c + ('a' - 'A');
}

[[nodiscard]] inline std::string lowercased(std::string s) {
    std::ranges::transform(std::move(s), begin(s), [](char c) { return lowercased(c); });
    return s;
}

constexpr bool no_case_compare(std::string_view a, std::string_view b) {
    return std::ranges::equal(a, b, [](auto c1, auto c2) { return lowercased(c1) == lowercased(c2); });
}

inline std::vector<std::string_view> split(std::string_view str, std::string_view sep) {
    std::vector<std::string_view> result{};
    for (auto p = str.find(sep); p != str.npos; p = str.find(sep)) {
        result.push_back(str.substr(0, p));
        str.remove_prefix(std::min(p + sep.size(), str.size()));
    }
    result.push_back(str);
    return result;
}

constexpr std::pair<std::string_view, std::string_view> split_once(std::string_view str, std::string_view sep) {
    if (auto p = str.find(sep); p != str.npos) {
        return {str.substr(0, p), str.substr(p + sep.size())};
    }
    return {str, ""};
}

constexpr bool is_whitespace(char ch) {
    constexpr std::array ws_chars = {' ', '\n', '\r', '\f', '\v', '\t'};
    return std::ranges::any_of(ws_chars, [ch](char ws_ch) { return ch == ws_ch; });
}

constexpr std::string_view trim_start(std::string_view s) {
    auto it = std::ranges::find_if(s, [](char ch) { return !is_whitespace(ch); });
    s.remove_prefix(std::distance(cbegin(s), it));
    return s;
}

constexpr std::string_view trim_end(std::string_view s) {
    auto it = std::find_if(crbegin(s), crend(s), [](char ch) { return !is_whitespace(ch); });
    s.remove_suffix(std::distance(crbegin(s), it));
    return s;
}

constexpr std::string_view trim(std::string_view s) {
    s = trim_start(s);
    s = trim_end(s);
    return s;
}

// To-Do(zero-one): specify constexpr when we drop gcc-11 support
// https://url.spec.whatwg.org/#concept-ipv4-serializer
inline std::string ipv4_serialize(std::uint32_t addr) {
    std::string out = "";
    std::uint32_t n = addr;

    for (std::size_t i = 1; i < 5; i++) {
        out.insert(0, std::to_string(n % 256));

        if (i != 4) {
            out.insert(0, ".");
        }

        n >>= 8;
    }

    return out;
}

// To-Do(zero-one): specify constexpr when we drop gcc-11 support
// https://url.spec.whatwg.org/#concept-ipv6-serializer
inline std::string ipv6_serialize(std::span<std::uint16_t, 8> addr) {
    std::string out = "";

    std::size_t compress = 0;

    std::size_t longest_run = 1;
    std::size_t run = 1;

    // Set compress to the index of the longest run of 0 pieces
    for (std::size_t i = 1; i < 8; i++) {
        if (addr[i - 1] == 0 && addr[i] == 0) {
            run++;

            if (run > longest_run) {
                longest_run = run;
                compress = i - (run - 1);
            }
        } else {
            run = 1;
        }
    }

    bool ignore0 = false;

    for (std::size_t i = 0; i < 8; i++) {
        if (ignore0 && addr[i] == 0) {
            continue;
        } else if (ignore0) {
            ignore0 = false;
        }

        if (longest_run > 1 && compress == i) {
            if (i == 0) {
                out += "::";
            } else {
                out += ":";
            }

            ignore0 = true;

            continue;
        }

        out += fmt::format("{:x}", addr[i]);

        if (i != 7) {
            out += ":";
        }
    }

    return out;
}

} // namespace util

#endif
