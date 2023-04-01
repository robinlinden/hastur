// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "css/rule.h"

#include "css/media_query.h"
#include "css/property_id.h"

#include <sstream>
#include <utility>
#include <variant>

namespace css {

std::string to_string(Rule const &rule) {
    std::stringstream ss;
    ss << "Selectors:";
    bool is_first_selector = true;
    for (auto const &selector : rule.selectors) {
        if (is_first_selector) {
            ss << " " << selector;
            is_first_selector = false;
        } else {
            ss << ", " << selector;
        }
    }
    ss << '\n';
    ss << "Declarations:\n";
    for (auto const &[property, value] : rule.declarations) {
        ss << "  " << to_string(property) << ": " << value << '\n';
    }
    if (rule.media_query.has_value()) {
        // TODO(robinlinden): to_string for media queries.
        auto query = std::get<MediaQuery::Width>(rule.media_query->query);
        ss << "Media query:\n";
        ss << "  " << query.min << " <= width <= " << query.max << '\n';
    }
    return std::move(ss).str();
}

} // namespace css
