// SPDX-FileCopyrightText: 2021-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef STYLE_STYLE_H_
#define STYLE_STYLE_H_

#include "css/media_query.h"
#include "css/property_id.h"
#include "css/style_sheet.h"
#include "dom/dom.h"
#include "style/styled_node.h"

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace style {

bool is_match(StyledNode const &, std::string_view selector);

struct MatchingProperties {
    std::vector<std::pair<css::PropertyId, std::string>> normal;
};

MatchingProperties matching_properties(StyledNode const &, css::StyleSheet const &, css::MediaQuery::Context const &);

std::unique_ptr<StyledNode> style_tree(
        dom::Node const &root, css::StyleSheet const &, css::MediaQuery::Context const & = {});

} // namespace style

#endif
