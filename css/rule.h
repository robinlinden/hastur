// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef CSS_RULE_H_
#define CSS_RULE_H_

#include "css/property_id.h"

#include <map>
#include <string>
#include <vector>

namespace css {

struct Rule {
    std::vector<std::string> selectors;
    std::map<PropertyId, std::string> declarations;
    std::string media_query;
    bool important{false};
    [[nodiscard]] bool operator==(Rule const &) const = default;
};

struct StyleSheet {
    std::vector<Rule> rules;
};


std::string to_string(Rule const &);

} // namespace css

#endif
