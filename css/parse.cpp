// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "css/parse.h"

#include "css/parser.h"

namespace css {

std::vector<css::Rule> parse(std::string_view input) {
    return css::Parser{input}.parse_rules();
}

} // namespace css
