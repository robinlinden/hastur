// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef STYLE_STYLE_H_
#define STYLE_STYLE_H_

#include "css/rule.h"
#include "dom/dom.h"
#include "style/styled_node.h"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace style {

bool is_match(dom::Element const &element, std::string_view selector);

std::vector<std::pair<std::string, std::string>> matching_rules(
        dom::Element const &element, std::vector<css::Rule> const &stylesheet);

StyledNode style_tree(dom::Node const &root, std::vector<css::Rule> const &stylesheet);

} // namespace style

#endif
