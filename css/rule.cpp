// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "css/rule.h"

#include <sstream>

namespace css {

std::string to_string(Rule const &rule) {
    std::stringstream ss;
    ss << "Selectors:";
    bool is_first_selector = true;
    for (auto const &selector : rule.selectors) {
        if (is_first_selector) {
            ss << " " << selector;
            is_first_selector = false;
        } else {
            ss << ", " << selector;
        }
    }
    ss << '\n';
    ss << "Declarations:\n";
    for (auto const &[property, value] : rule.declarations) {
        ss << "  " << property << ": " << value << '\n';
    }
    if (!rule.media_query.empty()) {
        ss << "Media query:\n";
        ss << "  " << rule.media_query << '\n';
    }
    return ss.str();
}

} // namespace css
