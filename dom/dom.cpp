// SPDX-FileCopyrightText: 2021-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "dom/dom.h"

#include <ostream>
#include <ranges>
#include <sstream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace dom {
namespace {

void print_whitespace(std::ostream &os, int depth) {
    if (depth > 0) {
        os << "\n| ";
    }

    for (int i = 1; i < depth; ++i) {
        os << "  ";
    }
}

void print_attribute(std::pair<std::string, std::string> const &attribute, std::ostream &os, int depth) {
    print_whitespace(os, depth);
    os << attribute.first << "=\"" << attribute.second << '"';
}

void print_node(dom::Node const &node, std::ostream &os, int initial_depth = 0) {
    std::vector<std::pair<dom::Node const *, int>> to_print{{&node, initial_depth}};
    while (!to_print.empty()) {
        auto [current_node, current_depth] = to_print.back();
        to_print.pop_back();

        print_whitespace(os, current_depth);

        if (auto const *element = std::get_if<dom::Element>(current_node)) {
            os << '<' << element->name << ">";
            for (auto const &attribute : element->attributes) {
                print_attribute(attribute, os, current_depth + 1);
            }

            for (auto const &child : element->children | std::views::reverse) {
                to_print.emplace_back(&child, current_depth + 1);
            }
        } else if (auto const *comment = std::get_if<dom::Comment>(current_node)) {
            os << "<!-- " << comment->text << " -->";
        } else {
            auto const &text = std::get<dom::Text>(*current_node);
            os << '"' << text.text << '"';
        }
    }
}

} // namespace

std::string to_string(Document const &document) {
    std::stringstream ss;
    ss << "#document";
    for (auto const &comment : document.pre_html_node_comments) {
        ss << "\n| <!-- " << comment.text << " -->";
    }

    if (!document.doctype.empty()) {
        ss << "\n| <!DOCTYPE " << document.doctype;
        if (!document.public_identifier.empty() || !document.system_identifier.empty()) {
            ss << " \"" << document.public_identifier << "\" \"" << document.system_identifier << '"';
        }
        ss << '>';
    }

    print_node(document.html_node, ss, 1);
    return std::move(ss).str();
}

std::string to_string(Node const &element) {
    std::stringstream ss;
    print_node(element, ss);
    return std::move(ss).str();
}

} // namespace dom
