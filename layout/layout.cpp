#include "layout/layout.h"

#include <algorithm>
#include <cassert>
#include <charconv>
#include <optional>
#include <sstream>
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
    }, node.node.get().data);
}

// TODO(robinlinden):
// * margin, border, padding, etc.
// * Not all measurements have to be in pixels.
int to_px(std::string_view property) {
    int res{};
    std::from_chars(property.data(), property.data() + property.size(), res);
    if (property.ends_with("em")) {
        // TODO(robinlinden): Value based on font-size for current node.
        res *= 10;
    }

    return res;
}

void calculate_width(LayoutBox &box, Rect const &parent) {
    assert(box.node != nullptr);
    auto width = get_property_or(*box.node, "width", "auto");
    int width_px = width == "auto" ? static_cast<int>(parent.width) : to_px(width);

    if (auto min = get_property(*box.node, "min-width"); min) {
        width_px = std::max(width_px, to_px(*min));
    }

    if (auto max = get_property(*box.node, "max-width"); max) {
        width_px = std::min(width_px, to_px(*max));
    }

    int underflow = static_cast<int>(parent.width) - width_px;
    if (underflow < 0) {
        // Overflow, this should adjust the right margin, but for now...
        width_px += underflow;
    }

    box.dimensions.content.width = static_cast<float>(width_px);
}

void calculate_position(LayoutBox &box, Rect const &parent) {
    box.dimensions.content.x = parent.x;
    // Position below previous content in parent.
    box.dimensions.content.y = parent.y + parent.height;
}

void calculate_height(LayoutBox &box) {
    assert(box.node != nullptr);
    if (auto height = get_property(*box.node, "height"); height) {
        box.dimensions.content.height = static_cast<float>(to_px(*height));
    }

    if (auto min = get_property(*box.node, "min-height"); min) {
        box.dimensions.content.height = std::max(box.dimensions.content.height, static_cast<float>(to_px(*min)));
    }

    if (auto max = get_property(*box.node, "max-height"); max) {
        box.dimensions.content.height = std::min(box.dimensions.content.height, static_cast<float>(to_px(*max)));
    }
}

void layout(LayoutBox &box, Rect const &bounds) {
    switch (box.type) {
        case LayoutType::Inline:
        case LayoutType::Block: {
            calculate_width(box, bounds);
            calculate_position(box, bounds);
            for (auto &child : box.children) {
                layout(child, box.dimensions.content);
                box.dimensions.content.height += child.dimensions.content.height;
            }
            calculate_height(box);
            return;
        }
        // TODO(robinlinden): This needs to place its children side-by-side.
        case LayoutType::AnonymousBlock: {
            calculate_position(box, bounds);
            for (auto &child : box.children) {
                layout(child, box.dimensions.content);
                box.dimensions.content.height += child.dimensions.content.height;
            }
            return;
        }
    }
}

std::string_view to_str(LayoutType type) {
    switch (type) {
        case LayoutType::Inline: return "inline";
        case LayoutType::Block: return "block";
        case LayoutType::AnonymousBlock: return "ablock";
    }
    assert(false);
    std::abort();
}

std::string_view to_str(dom::Node const &node) {
    return std::visit(Overloaded {
        [](dom::Element const &element) -> std::string_view { return element.name; },
        [](dom::Text const &text) -> std::string_view { return text.text; },
    }, node.data);
}

std::string to_str(Rect const &rect) {
    std::stringstream ss;
    ss << "{" << rect.x << "," << rect.y << "," << rect.width << "," << rect.height << "}";
    return ss.str();
}

void print_box(LayoutBox const &box, std::ostream &os, uint8_t depth = 0) {
    for (int8_t i = 0; i < depth; ++i) { os << "  "; }

    if (box.node != nullptr) {
        os << to_str(box.node->node.get()) << '\n';
        for (int8_t i = 0; i < depth; ++i) { os << "  "; }
    }

    os << to_str(box.type) << " " << to_str(box.dimensions.content) << '\n';
    for (auto const &child : box.children) { print_box(child, os, depth + 1); }
}

} // namespace

LayoutBox create_layout(style::StyledNode const &node, int width) {
    auto tree = create_tree(node);
    layout(*tree, {0, 0, static_cast<float>(width), 0});
    return *tree;
}

std::string to_string(LayoutBox const &box) {
    std::stringstream ss;
    print_box(box, ss);
    return ss.str();
}

} // namespace layout
