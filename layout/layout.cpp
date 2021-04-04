#include "layout/layout.h"

#include <algorithm>
#include <optional>
#include <utility>
#include <variant>

namespace layout {
namespace {

template<class... Ts>
struct Overloaded : Ts... { using Ts::operator()...; };

// Not needed as of C++20, but gcc 10 won't work without it.
template<class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

bool last_node_was_anonymous(LayoutBox const &box) {
    return !box.children.empty() && box.children.back().type == LayoutType::AnonymousBlock;
}

std::optional<std::string_view> get_property(
        style::StyledNode const &node,
        std::string_view property) {
    auto it = std::find_if(cbegin(node.properties), cend(node.properties), [=](auto const &p) {
        return p.first == property;
    });

    if (it == cend(node.properties)) {
        return std::nullopt;
    }

    return it->second;
}

std::optional<LayoutBox> create_layout_tree(style::StyledNode const &node) {
    return std::visit(Overloaded {
        [](dom::Doctype const &) -> std::optional<LayoutBox> { return std::nullopt; },
        [&node](dom::Element const &) -> std::optional<LayoutBox> {
            auto display = get_property(node, "display");
            if (display && *display == "none") {
                return std::nullopt;
            }

            LayoutBox box{&node, display == "inline" ? LayoutType::Inline : LayoutType::Block};

            for (auto const &child : node.children) {
                auto child_box = create_layout_tree(child);
                if (!child_box) continue;

                if (child_box->type == LayoutType::Inline) {
                    if (!last_node_was_anonymous(box)) {
                        box.children.push_back(LayoutBox{nullptr, LayoutType::AnonymousBlock});
                    }

                    box.children.back().children.push_back(std::move(*child_box));
                } else {
                    box.children.push_back(std::move(*child_box));
                }
            }

            return box;
        },
        [&node](dom::Text const &) -> std::optional<LayoutBox> {
            return LayoutBox{&node, LayoutType::Inline};
        },
    }, node.node.data);
}

} // namespace

LayoutBox create_layout(style::StyledNode const &node) {
    return *create_layout_tree(node);
}

} // namespace layout
