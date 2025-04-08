// SPDX-FileCopyrightText: 2021-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef LAYOUT_LAYOUT_BOX_H_
#define LAYOUT_LAYOUT_BOX_H_

#include "layout/box_model.h"

#include "css/property_id.h"
#include "dom/dom.h"
#include "geom/geom.h"
#include "style/styled_node.h"

#include <cassert>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace layout {

// NOLINTNEXTLINE(misc-no-recursion)
struct LayoutBox {
    style::StyledNode const *node;
    BoxModel dimensions;
    std::vector<LayoutBox> children;
    std::variant<std::monostate, std::string_view, std::string> layout_text;
    // NOLINTNEXTLINE(misc-no-recursion)
    [[nodiscard]] bool operator==(LayoutBox const &) const = default;

    bool is_anonymous_block() const { return node == nullptr; }
    std::optional<std::string_view> text() const;

    template<css::PropertyId T>
    auto get_property() const {
        if (is_anonymous_block()) {
            if (css::is_inherited(T)) {
                // TODO(robinlinden): Sad roundabout way of getting the parent.
                // Make this nicer.
                assert(!children.empty());
                auto const *child = children.front().node;
                assert(child != nullptr && child->parent != nullptr);
                return child->parent->get_property<T>();
            }

            return style::initial_value<T>();
        }

        assert(node);
        return node->get_property<T>();
    }
};

LayoutBox const *box_at_position(LayoutBox const &, geom::Position);

std::string to_string(LayoutBox const &box);

inline std::string_view dom_name(LayoutBox const &node) {
    assert(node.node);
    return std::get<dom::Element>(node.node->node).name;
}

inline std::vector<LayoutBox const *> dom_children(LayoutBox const &node) {
    assert(node.node);
    std::vector<LayoutBox const *> children{};
    for (auto const &child : node.children) {
        if (child.is_anonymous_block()) {
            for (auto const &inline_child : child.children) {
                assert(inline_child.node);
                if (!std::holds_alternative<dom::Element>(inline_child.node->node)) {
                    continue;
                }

                children.push_back(&inline_child);
            }
            continue;
        }

        assert(child.node);
        if (!std::holds_alternative<dom::Element>(child.node->node)) {
            continue;
        }

        children.push_back(&child);
    }
    return children;
}

} // namespace layout

#endif // LAYOUT_LAYOUT_BOX_H_
