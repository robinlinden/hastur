// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef DOM_DOM_H_
#define DOM_DOM_H_

#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <utility>
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

inline Node create_element_node(std::string_view name, AttrMap attrs, std::vector<Node> children) {
    return Element{std::string{name}, std::move(attrs), std::move(children)};
}

// reference_wrapper to ensure that the argument isn't a temporary since we return pointers into it.
std::vector<Element const *> nodes_by_path(std::reference_wrapper<Node const>, std::string_view path);
std::vector<Element const *> nodes_by_path(std::reference_wrapper<Element const>, std::string_view path);

std::string to_string(Document const &node);

} // namespace dom

#endif
