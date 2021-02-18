#include "dom/dom.h"

#include <ostream>
#include <sstream>

namespace dom {
namespace {

template<class... Ts>
struct Overloaded : Ts... { using Ts::operator()...; };

// Not needed as of C++20, but gcc 10 won't work without it.
template<class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

void print_node(dom::Node node, std::ostream &os, uint8_t depth = 0) {
    for (int8_t i = 0; i < depth; ++i) { os << "  "; }
    std::visit(Overloaded {
        [&os](dom::Doctype const &doctype) { os << "doctype: " << doctype.doctype << '\n'; },
        [&os](dom::Element const &element) { os << "tag: " << element.name << '\n'; },
        [&os](dom::Text const &text) { os << "value: " << text.text << '\n'; },
    }, node.data);

    for (auto const &child : node.children) { print_node(child, os, depth + 1); }
}

} // namespace

std::string to_string(Node const &node) {
    std::stringstream ss;
    print_node(node, ss);
    return ss.str();
}

} // namespace dom
