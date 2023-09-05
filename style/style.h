// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef STYLE_STYLE_H_
#define STYLE_STYLE_H_

#include "css/media_query.h"
#include "css/property_id.h"
#include "css/rule.h"
#include "dom/dom.h"
#include "style/styled_node.h"

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace style {

bool is_match(StyledNode const &, std::string_view selector);

std::vector<std::pair<css::PropertyId, std::string>> matching_rules(
        StyledNode const &, std::vector<css::Rule> const &stylesheet, css::MediaQuery::Context const &);

inline bool is_match(dom::Element const &e, std::string_view selector) {
    return is_match(StyledNode{e}, selector);
}

inline std::vector<std::pair<css::PropertyId, std::string>> matching_rules(dom::Element const &element,
        std::vector<css::Rule> const &stylesheet,
        css::MediaQuery::Context const &context = {}) {
    return matching_rules(StyledNode{element}, stylesheet, context);
}

std::unique_ptr<StyledNode> style_tree(
        dom::Node const &root, std::vector<css::Rule> const &stylesheet, css::MediaQuery::Context const & = {});

} // namespace style

#endif
