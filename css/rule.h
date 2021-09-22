// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef CSS_RULE_H_
#define CSS_RULE_H_

#include <map>
#include <string>
#include <vector>

namespace css {

struct Rule {
    std::vector<std::string> selectors;
    std::map<std::string, std::string> declarations;
    std::string media_query;
};

} // namespace css

#endif
