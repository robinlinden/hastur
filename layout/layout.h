#ifndef LAYOUT_LAYOUT_H_
#define LAYOUT_LAYOUT_H_

#include "style/styled_node.h"

#include <string>
#include <vector>

namespace layout {

struct Rect {
    float x{}, y{}, width{}, height{};
    bool operator==(Rect const &) const = default;
};

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
    bool operator==(LayoutBox const &) const = default;
};

LayoutBox create_layout(style::StyledNode const &node, int width);

std::string to_string(LayoutBox const &box);

} // namespace layout

#endif
