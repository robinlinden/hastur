#include "layout/layout.h"

#include <algorithm>
#include <cassert>
#include <charconv>
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

std::string_view get_property_or(
        style::StyledNode const &node,
        std::string_view property,
        std::string_view fallback) {
    if (auto prop = get_property(node, property); prop) {
        return *prop;
    }
    return fallback;
}

std::optional<LayoutBox> create_tree(style::StyledNode const &node) {
    return std::visit(Overloaded {
        [](dom::Doctype const &) -> std::optional<LayoutBox> { return std::nullopt; },
        [&node](dom::Element const &) -> std::optional<LayoutBox> {
            auto display = get_property(node, "display");
            if (display && *display == "none") {
                return std::nullopt;
            }

            LayoutBox box{&node, display == "inline" ? LayoutType::Inline : LayoutType::Block};

            for (auto const &child : node.children) {
                auto child_box = create_tree(child);
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

// TODO(robinlinden):
// * Deal with more layout types.
// * margin, border, padding, etc.
// * Not all measurements have to be in pixels.
int to_px(std::string_view property) {
    int res{};
    std::from_chars(property.data(), property.data() + property.size(), res);
    return res;
}

void calculate_width(LayoutBox &box, Rect const &parent) {
    assert(box.node != nullptr);
    auto width = get_property_or(*box.node, "width", "auto");
    int width_px = width == "auto" ? static_cast<int>(parent.width) : to_px(width);
    int underflow = static_cast<int>(parent.width) - width_px;
    if (underflow < 0) {
        // Overflow, this should adjust the right margin, but for now...
        width_px += underflow;
    }

    box.dimensions.width = static_cast<float>(width_px);
}

void calculate_position(LayoutBox &box, Rect const &parent) {
    box.dimensions.x = parent.x;
    // Position below previous content in parent.
    box.dimensions.y = parent.y + parent.height;
}

// The box should already have the correct height unless it's overridden in CSS.
void calculate_height(LayoutBox &box) {
    assert(box.node != nullptr);
    if (auto height = get_property(*box.node, "height"); height) {
        box.dimensions.height = static_cast<float>(to_px(*height));
    }
}

void layout(LayoutBox &box, Rect const &bounds) {
    switch (box.type) {
        case LayoutType::Block: {
            calculate_width(box, bounds);
            calculate_position(box, bounds);
            for (auto &child : box.children) {
                layout(child, box.dimensions);
                box.dimensions.height += child.dimensions.height;
            }
            calculate_height(box);
            return;
        }
        case LayoutType::Inline: return;
        case LayoutType::AnonymousBlock: return;
    }
}

} // namespace

LayoutBox create_layout(style::StyledNode const &node, int width) {
    auto tree = create_tree(node);
    layout(*tree, {0, 0, static_cast<float>(width), 0});
    return *tree;
}

} // namespace layout
