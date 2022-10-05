// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2021 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef UTIL_STRING_H_
#define UTIL_STRING_H_

#include <algorithm>
#include <array>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

namespace util {

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

inline std::pair<std::string_view, std::string_view> split_once(std::string_view str, std::string_view sep) {
    if (auto p = str.find(sep); p != str.npos) {
        return {str.substr(0, p), str.substr(p + sep.size(), str.size() - p - sep.size())};
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

} // namespace util

#endif
