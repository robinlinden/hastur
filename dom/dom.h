// SPDX-FileCopyrightText: 2021-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef DOM_DOM_H_
#define DOM_DOM_H_

#include <cstdint>
#include <functional>
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

    // https://dom.spec.whatwg.org/#concept-document-mode
    enum class Mode : std::uint8_t {
        NoQuirks,
        Quirks,
        LimitedQuirks,
    } mode{};

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

std::string to_string(Document const &);

} // namespace dom

#endif
