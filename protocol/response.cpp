// SPDX-FileCopyrightText: 2021-2025 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2021-2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "protocol/response.h"

#include "util/string.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace protocol {
namespace detail {

bool CaseInsensitiveLess::operator()(std::string_view s1, std::string_view s2) const {
    return std::ranges::lexicographical_compare(
            s1, s2, [](char c1, char c2) { return util::lowercased(c1) < util::lowercased(c2); });
}

} // namespace detail

std::string_view to_string(ErrorCode e) {
    switch (e) {
        case ErrorCode::Unresolved:
            return "Unresolved";
        case ErrorCode::Unhandled:
            return "Unhandled";
        case ErrorCode::InvalidResponse:
            return "InvalidResponse";
        case ErrorCode::RedirectLimit:
            return "RedirectLimit";
    }
    return "Unknown";
}

std::string to_string(Headers const &h) {
    std::stringstream ss;
    for (auto const &[name, value] : h) {
        ss << name << ": " << value << "\n";
    }
    return std::move(ss).str();
}

} // namespace protocol
