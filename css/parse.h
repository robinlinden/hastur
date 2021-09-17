// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef CSS_PARSE_H_
#define CSS_PARSE_H_

#include "css/rule.h"

#include <string_view>
#include <vector>

namespace css {

std::vector<css::Rule> parse(std::string_view input);

} // namespace css

#endif
