// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "dom/dom.h"

#include "util/overloaded.h"

#include <algorithm>
#include <iterator>
#include <ostream>
#include <sstream>
#include <utility>
#include <variant>
#include <vector>

namespace dom {
namespace {

void print_node(dom::Node const &node, std::ostream &os, uint8_t depth = 0) {
    for (int8_t i = 0; i < depth; ++i) {
        os << "  ";
    }
    std::visit(util::Overloaded{
                       [&os, depth](dom::Element const &element) {
                           os << "tag: " << element.name << '\n';
                           for (auto const &child : element.children) {
                               print_node(child, os, depth + 1);
                           }
                       },
                       [&os](dom::Text const &text) { os << "value: " << text.text << '\n'; },
               },
            node);
}

} // namespace

std::string to_string(Document const &document) {
    std::stringstream ss;
    ss << "doctype: " << document.doctype << '\n';
    print_node(document.html_node, ss);
    return std::move(ss).str();
}

// https://developer.mozilla.org/en-US/docs/Web/XPath
// https://en.wikipedia.org/wiki/XPath
std::vector<Element const *> nodes_by_xpath(Element const &root, std::string_view xpath) {
    std::vector<Element const *> next_search{&root};
    std::vector<Element const *> searching{};
    std::vector<Element const *> goal_nodes{};

    // We only support xpaths in the form /a/b/c right now.
    if (!xpath.starts_with('/')) {
        return {};
    }
    xpath.remove_prefix(1);

    while (!next_search.empty()) {
        searching.swap(next_search);
        next_search.clear();

        for (auto node : searching) {
            if (xpath == node->name) {
                goal_nodes.push_back(node);
                continue;
            }

            if (xpath.starts_with(node->name + "/")) {
                for (auto const &child : node->children) {
                    if (auto const *element = std::get_if<Element>(&child)) {
                        next_search.push_back(element);
                    }
                }
            }
        }

        // Remove name + separator.
        std::size_t separator_position{xpath.find_first_of("/")};
        if (separator_position == xpath.npos) {
            break;
        }

        xpath.remove_prefix(separator_position + 1);
    }

    return goal_nodes;
}

} // namespace dom
