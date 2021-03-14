#include "style/style.h"

#include <algorithm>
#include <iterator>
#include <utility>

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

StyledNode style_tree(dom::Node const &root, std::vector<css::Rule> const &stylesheet) {
    std::vector<StyledNode> children{};
    for (auto const &child : root.children) {
        children.push_back(style_tree(child, stylesheet));
    }

    auto properties = std::holds_alternative<dom::Element>(root.data)
            ? matching_rules(std::get<dom::Element>(root.data), stylesheet)
            : std::vector<std::pair<std::string, std::string>>{};

    return {
        .node = root,
        .properties = std::move(properties),
        .children = std::move(children),
    };
}

} // namespace style
