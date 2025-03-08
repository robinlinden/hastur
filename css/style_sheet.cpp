// SPDX-FileCopyrightText: 2022-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "css/style_sheet.h"

#include "css/rule.h"

#include <sstream>
#include <string>
#include <utility>

namespace css {

std::string to_string(css::StyleSheet const &stylesheet) {
    std::stringstream ss;
    for (auto const &rule : stylesheet.rules) {
        ss << to_string(rule) << '\n';
    }
    return std::move(ss).str();
}

} // namespace css
