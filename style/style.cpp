// SPDX-FileCopyrightText: 2021-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "style/style.h"

#include "style/styled_node.h"

#include "css/media_query.h"
#include "css/parser.h"
#include "css/property_id.h"
#include "css/style_sheet.h"
#include "dom/dom.h"
#include "util/string.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <iterator>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

using namespace std::literals;

namespace style {
namespace {
bool contains_class(std::string_view classes, std::string_view needle_class) {
    for (auto cls : classes | std::views::split(' ')) {
        if (std::string_view{cls} == needle_class) {
            return true;
        }
    }

    return false;
}
} // namespace

// TODO(robinlinden): This needs to match more things.
// NOLINTNEXTLINE(misc-no-recursion)
bool is_match(style::StyledNode const &node, std::string_view selector) {
    auto const &element = std::get<dom::Element>(node.node);
    // https://developer.mozilla.org/en-US/docs/Web/CSS/Pseudo-classes
    auto [selector_, psuedo_class] = util::split_once(selector, ':');

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
        for (auto part : parts) {
            part = util::trim(part);
            // TODO(robinlinden): Handle descendant and child combinators in the same selector.
            if (part.contains(' ')) {
                return false;
            }

            if (current == nullptr || !is_match(*current, part)) {
                return false;
            }

            current = current->parent;
        }
    }

    // https://developer.mozilla.org/en-US/docs/Web/CSS/Descendant_combinator
    if (selector_.contains(' ')) {
        // TODO(robinlinden): std::views::reverse and friends when we drop Clang 14 and 15.
        auto parts = util::split(selector_, " ");
        selector_ = util::trim(parts.back());
        parts.pop_back();
        std::ranges::reverse(parts);

        // We only check the parent and up here, and if they all match, we fall
        // through and check this node.
        auto const *current = node.parent;
        for (auto const &part : parts) {
            while (current != nullptr && !is_match(*current, util::trim(part))) {
                current = current->parent;
            }

            if (current == nullptr) {
                return false;
            }

            current = current->parent;
        }
    }

    if (!psuedo_class.empty()) {
        if (psuedo_class == "link" || psuedo_class == "any-link") {
            // https://developer.mozilla.org/en-US/docs/Web/CSS/:any-link
            // https://developer.mozilla.org/en-US/docs/Web/CSS/:link
            // https://developer.mozilla.org/en-US/docs/Web/CSS/:visited
            // Ignoring :visited for now as we treat all links as unvisited.
            if (!element.attributes.contains("href")) {
                return false;
            }

            if (element.name != "a" && element.name != "area") {
                return false;
            }

            if (selector_.empty()) {
                return true;
            }
        } else if (psuedo_class == "root") {
            // https://developer.mozilla.org/en-US/docs/Web/CSS/:root
            if (node.parent != nullptr) {
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

    auto class_position = selector_.find('.');
    if (class_position != std::string_view::npos) {
        auto class_attr = element.attributes.find("class");
        if (class_attr == element.attributes.end()) {
            return false;
        }

        auto class_string = selector_.substr(class_position);
        if (class_position != 0 && selector_.substr(0, class_position) != element.name) {
            return false;
        }

        class_string.remove_prefix(1);
        for (auto cls : class_string | std::views::split('.')) {
            if (!contains_class(class_attr->second, std::string_view{cls})) {
                return false;
            }
        }

        return true;
    }

    if (selector_.starts_with('#')) {
        auto it = element.attributes.find("id");
        selector_.remove_prefix(1);
        return it != element.attributes.end() && it->second == selector_;
    }

    // https://developer.mozilla.org/en-US/docs/Web/CSS/Attribute_selectors
    if (selector_.starts_with('[') && selector_.contains(']')) {
        selector_.remove_prefix(1);
        auto [attr, rest] = util::split_once(selector_, ']');
        if (!rest.empty() && !is_match(node, rest)) {
            return false;
        }

        auto [key, value] = util::split_once(attr, '=');
        if (value.empty()) {
            return element.attributes.contains(key);
        }

        auto it = element.attributes.find(key);
        return it != element.attributes.end() && it->second == value;
    }

    return false;
}

MatchingProperties matching_properties(
        style::StyledNode const &node, css::StyleSheet const &stylesheet, css::MediaQuery::Context const &ctx) {
    std::vector<std::pair<css::PropertyId, std::string>> matched_properties;
    std::vector<std::pair<std::string, std::string>> matched_custom_properties;

    for (auto const &rule : stylesheet.rules) {
        if (rule.media_query.has_value() && !rule.media_query->evaluate(ctx)) {
            continue;
        }

        if (std::ranges::any_of(rule.selectors, [&](auto const &selector) { return is_match(node, selector); })) {
            std::ranges::copy(rule.declarations, std::back_inserter(matched_properties));
            std::ranges::copy(rule.custom_properties, std::back_inserter(matched_custom_properties));
        }
    }

    if (auto const *element = std::get_if<dom::Element>(&node.node)) {
        auto style_attr = element->attributes.find("style");
        if (style_attr != element->attributes.end()) {
            // TODO(robinlinden): Incredibly hacky, but our //css parser doesn't support
            // parsing only declarations. Replace with the //css2 parser once possible.
            auto element_style = css::parse("dummy{"s + style_attr->second + "}"s).rules;
            // The above should always parse to 1 rule when using the old parser.
            if (element_style.size() == 1) {
                std::ranges::copy(element_style[0].declarations, std::back_inserter(matched_properties));
                std::ranges::copy(element_style[0].important_declarations, std::back_inserter(matched_properties));
                std::ranges::copy(element_style[0].custom_properties, std::back_inserter(matched_custom_properties));
            } else {
                spdlog::warn("Failed to parse inline style '{}' for element '{}'", style_attr->second, element->name);
            }
        }
    }

    // TODO(robinlinden): !important inline styles should override the ones from
    // the style sheets.
    for (auto const &rule : stylesheet.rules) {
        if (rule.important_declarations.empty() || (rule.media_query.has_value() && !rule.media_query->evaluate(ctx))) {
            continue;
        }

        if (std::ranges::any_of(rule.selectors, [&](auto const &selector) { return is_match(node, selector); })) {
            std::ranges::copy(rule.important_declarations, std::back_inserter(matched_properties));
        }
    }

    return {std::move(matched_properties), std::move(matched_custom_properties)};
}

namespace {
// NOLINTNEXTLINE(misc-no-recursion)
void style_tree_impl(StyledNode &current, css::StyleSheet const &stylesheet, css::MediaQuery::Context const &ctx) {
    auto const *element = std::get_if<dom::Element>(&current.node);
    if (element == nullptr) {
        return;
    }

    current.children.reserve(element->children.size());
    for (auto const &child : element->children) {
        auto &child_node = current.children.emplace_back(child);
        child_node.parent = &current;
        style_tree_impl(child_node, stylesheet, ctx);
    }

    auto [normal, custom] = matching_properties(current, stylesheet, ctx);
    current.properties = std::move(normal);
    current.custom_properties = std::move(custom);
}
} // namespace

std::unique_ptr<StyledNode> style_tree(
        dom::Node const &root, css::StyleSheet const &stylesheet, css::MediaQuery::Context const &ctx) {
    auto tree_root = std::make_unique<StyledNode>(root);
    style_tree_impl(*tree_root, stylesheet, ctx);
    return tree_root;
}

} // namespace style
