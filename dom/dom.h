// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef DOM_DOM_H_
#define DOM_DOM_H_

#include <cstddef>
#include <map>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace dom {

struct Text;
struct Element;

using AttrMap = std::map<std::string, std::string, std::less<>>;
using Node = std::variant<Element, Text>;

struct Text {
    std::string text;
    [[nodiscard]] bool operator==(Text const &) const = default;
};

struct Element {
    std::string name;
    AttrMap attributes;
    std::vector<Node> children;
    [[nodiscard]] bool operator==(Element const &) const = default;
};

struct Document {
    std::string doctype;
    Node html_node;
    Element const &html() const { return std::get<Element>(html_node); }
    Element &html() { return std::get<Element>(html_node); }
    [[nodiscard]] bool operator==(Document const &) const = default;
};

inline std::string_view dom_name(Element const &e) {
    return e.name;
}

inline std::vector<Element const *> dom_children(Element const &e) {
    std::vector<Element const *> children;
    for (auto const &child : e.children) {
        if (auto const *element = std::get_if<Element>(&child)) {
            children.push_back(element);
        }
    }
    return children;
}

// https://developer.mozilla.org/en-US/docs/Web/XPath
// https://en.wikipedia.org/wiki/XPath
template<typename T>
inline std::vector<T const *> nodes_by_xpath(T const &root, std::string_view xpath) {
    std::vector<T const *> next_search{&root};
    std::vector<T const *> searching{};
    std::vector<T const *> goal_nodes{};

    // We only support xpaths in the form /a/b/c right now.
    if (!xpath.starts_with('/')) {
        return {};
    }

    auto search_children = [&] {
        xpath.remove_prefix(1);
        for (auto node : searching) {
            auto name = dom_name(*node);
            if (xpath == name) {
                goal_nodes.push_back(node);
                continue;
            }

            if (xpath.starts_with(name) && xpath.size() >= name.size() + 1 && xpath[name.size()] == '/') {
                for (auto const *child : dom_children(*node)) {
                    next_search.push_back(child);
                }
            }
        }

        // Remove name.
        std::size_t separator_position{xpath.find_first_of("/")};
        if (separator_position == xpath.npos) {
            xpath = std::string_view{};
            return;
        }

        xpath.remove_prefix(separator_position);
    };

    while (!next_search.empty() && !xpath.empty()) {
        searching.swap(next_search);
        next_search.clear();
        search_children();
    }

    return goal_nodes;
}

std::string to_string(Document const &node);

} // namespace dom

#endif
