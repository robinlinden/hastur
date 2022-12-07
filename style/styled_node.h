// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef STYLE_STYLED_NODE_H_
#define STYLE_STYLED_NODE_H_

#include "css/property_id.h"
#include "dom/dom.h"
#include "util/string.h"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace style {

enum class DisplayValue {
    None,
    Inline,
    Block,
};

struct StyledNode {
    dom::Node const &node;
    std::vector<std::pair<css::PropertyId, std::string>> properties;
    std::vector<StyledNode> children;
    StyledNode const *parent{nullptr};

    std::string_view get_raw_property(css::PropertyId) const;

    template<css::PropertyId T>
    auto get_property() const {
        if constexpr (T == css::PropertyId::Display) {
            return get_display_property(T);
        } else if constexpr (T == css::PropertyId::FontFamily) {
            auto font_family = get_raw_property(T);
            return util::split(font_family, ",");
        } else {
            return get_raw_property(T);
        }
    }

private:
    DisplayValue get_display_property(css::PropertyId) const;
};

[[nodiscard]] inline bool operator==(style::StyledNode const &a, style::StyledNode const &b) noexcept {
    return a.node == b.node && a.properties == b.properties && a.children == b.children;
}

} // namespace style

#endif
