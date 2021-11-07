// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2021 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef UTIL_STRING_H_
#define UTIL_STRING_H_

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

namespace util {

constexpr bool no_case_compare(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) {
        return false;
    }

    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(a[i]) != std::tolower(b[i])) {
            return false;
        }
    }

    return true;
}

inline std::string trim_start(std::string s) {
    s.erase(begin(s), std::find_if(begin(s), end(s), [](auto ch) { return !std::isspace(ch); }));
    return s;
}

inline std::string trim_end(std::string s) {
    s.erase(std::find_if(rbegin(s), rend(s), [](auto ch) { return !std::isspace(ch); }).base(), end(s));
    return s;
}

inline std::string trim(std::string s) {
    s = trim_start(std::move(s));
    s = trim_end(std::move(s));
    return s;
}

} // namespace util

#endif
