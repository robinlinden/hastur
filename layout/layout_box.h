// SPDX-FileCopyrightText: 2021-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef LAYOUT_LAYOUT_BOX_H_
#define LAYOUT_LAYOUT_BOX_H_

#include "layout/box_model.h"

#include "css/property_id.h"
#include "dom/dom.h"
#include "geom/geom.h"
#include "style/styled_node.h"
#include "style/unresolved_value.h"

#include <cassert>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace layout {

struct LayoutBox {
    style::StyledNode const *node;
    BoxModel dimensions;
    std::vector<LayoutBox> children;
    std::variant<std::monostate, std::string_view, std::string> layout_text;
    [[nodiscard]] bool operator==(LayoutBox const &) const = default;

    bool is_anonymous_block() const { return node == nullptr; }
    std::optional<std::string_view> text() const;

    template<css::PropertyId T>
    auto get_property() const {
        // Calling get_property on an anonymous block (the only type that
        // doesn't have a StyleNode) is a programming error.
        assert(!is_anonymous_block());
        assert(node);
        if constexpr (T == css::PropertyId::BorderBottomLeftRadius || T == css::PropertyId::BorderBottomRightRadius
                || T == css::PropertyId::BorderTopLeftRadius || T == css::PropertyId::BorderTopRightRadius) {
            return get_border_radius_property(T);
        } else if constexpr (T == css::PropertyId::MinWidth || T == css::PropertyId::Width
                || T == css::PropertyId::MaxWidth) {
            return style::UnresolvedValue{node->get_raw_property(T)};
        } else {
            return node->get_property<T>();
        }
    }

private:
    std::pair<int, int> get_border_radius_property(css::PropertyId) const;
    std::optional<int> get_min_width_property() const;
    std::optional<int> get_max_width_property() const;
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
