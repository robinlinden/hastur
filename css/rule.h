// SPDX-FileCopyrightText: 2021-2024 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef CSS_RULE_H_
#define CSS_RULE_H_

#include "css/media_query.h"
#include "css/property_id.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace css {

struct Rule {
    std::vector<std::string> selectors;
    std::map<PropertyId, std::string> declarations;
    std::map<PropertyId, std::string> important_declarations;
    std::map<std::string, std::string> custom_properties;
    std::optional<MediaQuery> media_query;
    [[nodiscard]] bool operator==(Rule const &) const = default;
};

std::string to_string(Rule const &);

} // namespace css

#endif
