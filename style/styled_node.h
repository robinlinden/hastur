#ifndef STYLE_STYLED_NODE_H_
#define STYLE_STYLED_NODE_H_

#include "dom/dom.h"

#include <string>
#include <utility>
#include <vector>

namespace style {

struct StyledNode {
    dom::Node const &node;
    std::vector<std::pair<std::string, std::string>> properties;
    std::vector<StyledNode> children;
};

inline bool operator==(style::StyledNode const &a, style::StyledNode const &b) noexcept {
    return a.node == b.node && a.properties == b.properties && a.children == b.children;
}

} // namespace style

#endif
