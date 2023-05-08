// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "dom/dom.h"

#include "util/overloaded.h"

#include <cstdint>
#include <ostream>
#include <sstream>
#include <utility>
#include <variant>

namespace dom {
namespace {

void print_node(dom::Node const &node, std::ostream &os, std::uint8_t depth = 0) {
    for (std::uint8_t i = 0; i < depth; ++i) {
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

} // namespace dom
