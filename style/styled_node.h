// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef STYLE_STYLED_NODE_H_
#define STYLE_STYLED_NODE_H_

#include "css/property_id.h"
#include "dom/dom.h"
#include "util/string.h"

#include <algorithm>
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

enum class FontStyle {
    Normal,
    Italic,
    Oblique,
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
            auto raw_font_family = get_raw_property(T);
            auto families = util::split(raw_font_family, ",");
            std::ranges::for_each(families, [](auto &family) { family = util::trim(family); });
            return families;
        } else if constexpr (T == css::PropertyId::FontSize) {
            return get_font_size_property();
        } else if constexpr (T == css::PropertyId::FontStyle) {
            return get_font_style_property();
        } else {
            return get_raw_property(T);
        }
    }

private:
    DisplayValue get_display_property(css::PropertyId) const;
    FontStyle get_font_style_property() const;
    int get_font_size_property() const;
};

[[nodiscard]] inline bool operator==(style::StyledNode const &a, style::StyledNode const &b) noexcept {
    return a.node == b.node && a.properties == b.properties && a.children == b.children;
}

} // namespace style

#endif
