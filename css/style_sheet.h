// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef CSS_STYLE_SHEET_H_
#define CSS_STYLE_SHEET_H_

#include "css/rule.h"

#include <vector>

namespace css {

struct StyleSheet {
    std::vector<Rule> rules;
    [[nodiscard]] bool operator==(StyleSheet const &) const = default;
};

} // namespace css

#endif
