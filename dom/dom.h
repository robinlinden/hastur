// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef DOM_DOM_H_
#define DOM_DOM_H_

#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace dom {

using AttrMap = std::map<std::string, std::string>;

struct Text {
    std::string text;
    bool operator==(Text const &) const = default;
};

struct Element {
    std::string name;
    AttrMap attributes;
    bool operator==(Element const &) const = default;
};

struct Node {
    std::vector<Node> children;
    std::variant<Text, Element> data;
    bool operator==(Node const &) const = default;
};

struct Document {
    std::string doctype;
    Node html;
    bool operator==(Document const &) const = default;
};

inline Document create_document(std::string_view doctype, Node html) {
    return {std::string{doctype}, std::move(html)};
}

inline Node create_text_node(std::string_view data) {
    return {{}, Text{std::string(data)}};
}

inline Node create_element_node(std::string_view name, AttrMap attrs, std::vector<Node> children) {
    return {std::move(children), Element{std::string{name}, std::move(attrs)}};
}

std::vector<Node const *> nodes_by_path(Node const &root, std::string_view path);

std::string to_string(Document const &node);

} // namespace dom

#endif
