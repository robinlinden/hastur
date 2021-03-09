#include "style/style.h"

#include <algorithm>
#include <iterator>

namespace style {

// TODO(robinlinden): This only matches simple names right now.
bool is_match(dom::Element const &element, std::string_view selector) {
    if (element.name == selector) {
        return true;
    }

    return false;
}

std::vector<std::pair<std::string, std::string>> matching_rules(
        dom::Element const &element,
        std::vector<css::Rule> const &stylesheet) {
    std::vector<std::pair<std::string, std::string>> matched_rules;

    for (auto const &rule : stylesheet) {
        if (std::any_of(rule.selectors.cbegin(), rule.selectors.cend(),
                [&](auto const &selector){ return is_match(element, selector); })) {
            std::copy(rule.declarations.cbegin(), rule.declarations.cend(),
                    std::back_inserter(matched_rules));
        }
    }

    return matched_rules;
}

} // namespace style
