// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "dom/dom.h"

#include <algorithm>
#include <iterator>
#include <ostream>
#include <sstream>
#include <variant>
#include <vector>

namespace dom {
namespace {

template<class... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};

// Not needed as of C++20, but gcc 10 won't work without it.
template<class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

void print_node(dom::Node const &node, std::ostream &os, uint8_t depth = 0) {
    for (int8_t i = 0; i < depth; ++i) {
        os << "  ";
    }
    std::visit(Overloaded{
                       [&os](dom::Element const &element) { os << "tag: " << element.name << '\n'; },
                       [&os](dom::Text const &text) { os << "value: " << text.text << '\n'; },
               },
            node.data);

    for (auto const &child : node.children) {
        print_node(child, os, depth + 1);
    }
}

} // namespace

std::string to_string(Document const &document) {
    std::stringstream ss;
    ss << "doctype: " << document.doctype << '\n';
    print_node(document.html, ss);
    return ss.str();
}

std::vector<Node const *> nodes_by_path(Node const &root, std::string_view path) {
    std::vector<Node const *> next_search{&root};
    std::vector<Node const *> searching{};
    std::vector<Node const *> goal_nodes{};

    while (!next_search.empty()) {
        searching.swap(next_search);
        next_search.clear();

        for (auto node : searching) {
            auto const *data = std::get_if<Element>(&node->data);
            if (!data) {
                continue;
            }

            if (path == data->name) {
                goal_nodes.push_back(node);
                continue;
            }

            if (path.starts_with(data->name + ".")) {
                std::transform(cbegin(node->children),
                        cend(node->children),
                        back_inserter(next_search),
                        [](Node const &n) -> Node const * { return &n; });
            }
        }

        // Remove name + separator.
        std::size_t separator_position{path.find_first_of(".")};
        if (separator_position == path.npos) {
            break;
        }

        path.remove_prefix(separator_position + 1);
    }

    return goal_nodes;
}

} // namespace dom
