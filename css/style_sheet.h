// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef CSS_STYLE_SHEET_H_
#define CSS_STYLE_SHEET_H_

#include "css/rule.h"

#include <iterator>
#include <vector>

namespace css {

struct StyleSheet {
    std::vector<Rule> rules;
    [[nodiscard]] bool operator==(StyleSheet const &) const = default;

    void splice(StyleSheet &&other) {
        rules.reserve(rules.size() + other.rules.size());
        rules.insert(
                end(rules), std::make_move_iterator(begin(other.rules)), std::make_move_iterator(end(other.rules)));
    }
};

} // namespace css

#endif
