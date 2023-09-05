// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "style/style.h"

#include "css/media_query.h"
#include "util/string.h"

#include <algorithm>
#include <iterator>
#include <utility>

namespace style {
namespace {
bool has_class(dom::Element const &element, std::string_view needle_class) {
    if (!element.attributes.contains("class")) {
        return false;
    }

    auto classes = util::split(element.attributes.at("class"), " ");
    return std::ranges::any_of(classes, [&](auto const &c) { return c == needle_class; });
}
} // namespace

// TODO(robinlinden): This needs to match more things.
bool is_match(style::StyledNode const &node, std::string_view selector) {
    auto const &element = std::get<dom::Element>(node.node);
    // https://developer.mozilla.org/en-US/docs/Web/CSS/Pseudo-classes
    auto [selector_, psuedo_class] = util::split_once(selector, ":");

    // https://developer.mozilla.org/en-US/docs/Web/CSS/Child_combinator
    if (selector_.contains('>')) {
        // TODO(robinlinden): std::views::reverse and friends when we drop Clang 14 and 15.
        auto parts = util::split(selector_, ">");
        selector_ = util::trim(parts.back());
        parts.pop_back();
        std::ranges::reverse(parts);

        // We only check the parent and up here, and if they all match, we fall
        // through and check this node.
        auto const *current = node.parent;
        for (auto const &part : parts) {
            if (current == nullptr || !is_match(*current, util::trim(part))) {
                return false;
            }

            current = current->parent;
        }
    }

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

            if (selector_.empty()) {
                return true;
            }
        } else {
            // Unhandled psuedo-classes never match.
            return false;
        }
    }

    // https://developer.mozilla.org/en-US/docs/Web/CSS/Universal_selectors
    if (selector_ == "*") {
        return true;
    }

    if (element.name == selector_) {
        return true;
    }

    if (selector_.starts_with('.')) {
        selector_.remove_prefix(1);
        return has_class(element, selector_);
    }

    if (selector_.starts_with('#') && element.attributes.contains("id")) {
        selector_.remove_prefix(1);
        return element.attributes.at("id") == selector_;
    }

    return false;
}

std::vector<std::pair<css::PropertyId, std::string>> matching_rules(
        style::StyledNode const &node, std::vector<css::Rule> const &stylesheet, css::MediaQuery::Context const &ctx) {
    std::vector<std::pair<css::PropertyId, std::string>> matched_rules;

    for (auto const &rule : stylesheet) {
        if (rule.media_query.has_value() && !rule.media_query->evaluate(ctx)) {
            continue;
        }

        if (std::ranges::any_of(rule.selectors, [&](auto const &selector) { return is_match(node, selector); })) {
            std::ranges::copy(rule.declarations, std::back_inserter(matched_rules));
        }
    }

    return matched_rules;
}

namespace {
void style_tree_impl(StyledNode &current,
        dom::Node const &root,
        std::vector<css::Rule> const &stylesheet,
        css::MediaQuery::Context const &ctx) {
    auto const *element = std::get_if<dom::Element>(&root);
    if (element == nullptr) {
        return;
    }

    current.children.reserve(element->children.size());
    for (auto const &child : element->children) {
        // TODO(robinlinden): emplace_back once Clang supports it (C++20/p0960). Not supported as of Clang 14.
        current.children.push_back({child});
        auto &child_node = current.children.back();
        child_node.parent = &current;
        style_tree_impl(child_node, child, stylesheet, ctx);
    }

    current.properties = matching_rules(current, stylesheet, ctx);
}
} // namespace

std::unique_ptr<StyledNode> style_tree(
        dom::Node const &root, std::vector<css::Rule> const &stylesheet, css::MediaQuery::Context const &ctx) {
    // TODO(robinlinden): std::make_unique once Clang supports it (C++20/p0960). Not supported as of Clang 14.
    auto tree_root = std::unique_ptr<StyledNode>(new StyledNode{root});
    style_tree_impl(*tree_root, root, stylesheet, ctx);
    return tree_root;
}

} // namespace style
