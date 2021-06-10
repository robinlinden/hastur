#ifndef LAYOUT_LAYOUT_H_
#define LAYOUT_LAYOUT_H_

#include "style/styled_node.h"

#include <string>
#include <vector>

namespace layout {

struct Rect {
    float x{}, y{}, width{}, height{};
};

constexpr bool operator==(Rect const &a, Rect const &b) noexcept {
    return a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height;
}

enum class LayoutType {
    Inline,
    Block,
    AnonymousBlock, // Holds groups of sequential inline boxes.
};

struct LayoutBox {
    style::StyledNode const *node;
    LayoutType type;
    Rect dimensions;
    std::vector<LayoutBox> children;
};

inline bool operator==(LayoutBox const &a, LayoutBox const &b) noexcept {
    return a.node == b.node
            && a.type == b.type
            && a.dimensions == b.dimensions
            && a.children == b.children;
}

LayoutBox create_layout(style::StyledNode const &node, int width);

std::string to_string(LayoutBox const &box);

} // namespace layout

#endif
