// SPDX-FileCopyrightText: 2021-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "dom/dom.h"

#include <cstdint>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>
#include <variant>

namespace dom {
namespace {

void print_attribute(std::pair<std::string, std::string> const &attribute, std::ostream &os, std::uint8_t depth) {
    os << "\n| ";
    for (std::uint8_t i = 1; i < depth; ++i) {
        os << "  ";
    }

    os << attribute.first << "=\"" << attribute.second << '"';
}

// NOLINTNEXTLINE(misc-no-recursion)
void print_node(dom::Node const &node, std::ostream &os, std::uint8_t depth = 0) {
    if (depth > 0) {
        os << "\n| ";
    }

    for (std::uint8_t i = 1; i < depth; ++i) {
        os << "  ";
    }

    if (auto const *element = std::get_if<dom::Element>(&node)) {
        os << '<' << element->name << ">";
        for (auto const &attribute : element->attributes) {
            print_attribute(attribute, os, depth + 1);
        }

        for (auto const &child : element->children) {
            print_node(child, os, depth + 1);
        }
    } else {
        os << '"' << std::get<dom::Text>(node).text << '"';
    }
}

} // namespace

std::string to_string(Document const &document) {
    std::stringstream ss;
    ss << "#document\n";
    if (!document.doctype.empty()) {
        ss << "| <!DOCTYPE " << document.doctype << '>';
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
