// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "style/style.h"

#include "util/string.h"

#include <algorithm>
#include <iterator>
#include <utility>

namespace style {

// TODO(robinlinden): This needs to match more things.
bool is_match(dom::Element const &element, std::string_view selector_) {
    // https://developer.mozilla.org/en-US/docs/Web/CSS/Pseudo-classes
    auto [selector, psuedo_class] = util::split_once(selector_, ":");

    if (!psuedo_class.empty()) {
        // https://developer.mozilla.org/en-US/docs/Web/CSS/:any-link
        // https://developer.mozilla.org/en-US/docs/Web/CSS/:link
        // https://developer.mozilla.org/en-US/docs/Web/CSS/:visited
        // Ignoring :visited for now as we treat all links as unvisited.
        if (psuedo_class == "link" || psuedo_class == "any-link") {
            if (!element.attributes.contains("href")) {
                return false;
            }

            if (element.name != "a" && element.name != "area") {
                return false;
            }

            if (selector.empty()) {
                return true;
            }
        } else {
            // Unhandled psuedo-classes never match.
            return false;
        }
    }

    if (element.name == selector) {
        return true;
    }

    if (selector.starts_with('.') && element.attributes.contains("class")) {
        selector.remove_prefix(1);
        return element.attributes.at("class") == selector;
    }

    if (selector.starts_with('#') && element.attributes.contains("id")) {
        selector.remove_prefix(1);
        return element.attributes.at("id") == selector;
    }

    return false;
}

std::vector<std::pair<css::PropertyId, std::string>> matching_rules(
        dom::Element const &element, std::vector<css::Rule> const &stylesheet) {
    std::vector<std::pair<css::PropertyId, std::string>> matched_rules;

    for (auto const &rule : stylesheet) {
        if (std::ranges::any_of(rule.selectors, [&](auto const &selector) { return is_match(element, selector); })) {
            std::ranges::copy(rule.declarations, std::back_inserter(matched_rules));
        }
    }

    return matched_rules;
}

namespace {
void style_tree_impl(StyledNode &current, dom::Node const &root, std::vector<css::Rule> const &stylesheet) {
    if (auto const *element = std::get_if<dom::Element>(&root)) {
        current.children.reserve(element->children.size());
        for (auto const &child : element->children) {
            // TODO(robinlinden): emplace_back once Clang supports it (C++20/p0960). Not supported as of Clang 14.
            current.children.push_back({child});
            auto &child_node = current.children.back();
            style_tree_impl(child_node, child, stylesheet);
            child_node.parent = &current;
        }
    }

    current.properties = std::holds_alternative<dom::Element>(root)
            ? matching_rules(std::get<dom::Element>(root), stylesheet)
            : std::vector<std::pair<css::PropertyId, std::string>>{};
}
} // namespace

std::unique_ptr<StyledNode> style_tree(dom::Node const &root, std::vector<css::Rule> const &stylesheet) {
    // TODO(robinlinden): std::make_unique once Clang supports it (C++20/p0960). Not supported as of Clang 14.
    auto tree_root = std::unique_ptr<StyledNode>(new StyledNode{root});
    style_tree_impl(*tree_root, root, stylesheet);
    return tree_root;
}

} // namespace style
