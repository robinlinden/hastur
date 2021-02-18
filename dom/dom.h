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

struct Doctype { std::string doctype; };

struct Text { std::string text; };

struct Element {
    std::string name;
    AttrMap attributes;
};

struct Node {
    std::vector<Node> children;
    std::variant<std::monostate, Doctype, Text, Element> data;
};

inline Node create_doctype_node(std::string_view doctype) {
    return {{}, Doctype{std::string{doctype}}};
}

inline Node create_text_node(std::string_view data) {
    return {{}, Text{std::string(data)}};
}

inline Node create_element_node(std::string_view name, AttrMap attrs, std::vector<Node> children) {
    return {std::move(children), Element{std::string{name}, std::move(attrs)}};
}

std::string to_string(Node const &node);

} // namespace dom

#endif
