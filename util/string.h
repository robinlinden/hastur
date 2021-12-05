// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2021 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef UTIL_STRING_H_
#define UTIL_STRING_H_

#include <range/v3/algorithm/equal.hpp>

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace util {

constexpr bool no_case_compare(std::string_view a, std::string_view b) {
    return ranges::equal(a, b, [](auto c1, auto c2) { return std::tolower(c1) == std::tolower(c2); });
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

inline std::string trim_start(std::string s) {
    s.erase(begin(s), std::find_if(begin(s), end(s), [](unsigned char ch) { return !std::isspace(ch); }));
    return s;
}

inline std::string trim_end(std::string s) {
    s.erase(std::find_if(rbegin(s), rend(s), [](unsigned char ch) { return !std::isspace(ch); }).base(), end(s));
    return s;
}

inline std::string trim(std::string s) {
    s = trim_start(std::move(s));
    s = trim_end(std::move(s));
    return s;
}

} // namespace util

#endif
