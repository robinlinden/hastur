// SPDX-FileCopyrightText: 2021-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef STYLE_STYLED_NODE_H_
#define STYLE_STYLED_NODE_H_

#include "style/unresolved_value.h"

#include "css/property_id.h"
#include "dom/dom.h"
#include "gfx/color.h"
#include "util/string.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace style {

enum class BorderStyle : std::uint8_t {
    None,
    Hidden,
    Dotted,
    Dashed,
    Solid,
    Double,
    Groove,
    Ridge,
    Inset,
    Outset,
};

using OutlineStyle = BorderStyle;

struct Display {
    enum class Outer : std::uint8_t {
        Inline,
        Block,
    };
    enum class Inner : std::uint8_t {
        Flow,
    };

    Outer outer{};
    Inner inner{};

    [[nodiscard]] constexpr bool operator==(Display const &) const = default;

    constexpr static Display inline_flow() { return {Outer::Inline, Inner::Flow}; }
    constexpr static Display block_flow() { return {Outer::Block, Inner::Flow}; }
};

enum class Float : std::uint8_t {
    None,
    Left,
    Right,
    InlineStart,
    InlineEnd,
};

enum class FontStyle : std::uint8_t {
    Normal,
    Italic,
    Oblique,
};

struct FontWeight {
    int value{};
    [[nodiscard]] constexpr bool operator==(FontWeight const &) const = default;
    static constexpr int kNormal = 400;
    static constexpr int kBold = 700;
    static constexpr FontWeight normal() { return {kNormal}; }
    static constexpr FontWeight bold() { return {kBold}; }
};

enum class TextAlign : std::uint8_t {
    Left,
    Right,
    Center,
    Justify,
};

enum class TextDecorationLine : std::uint8_t {
    None,
    Underline,
    Overline,
    LineThrough,
};

enum class TextTransform : std::uint8_t {
    None,
    Capitalize,
    Uppercase,
    Lowercase,
    FullWidth,
    FullSizeKana,
};

enum class WhiteSpace : std::uint8_t {
    Normal,
    Pre,
    Nowrap,
    PreWrap,
    BreakSpaces,
    PreLine,
};

struct UnresolvedBorderWidth {
    UnresolvedValue width{};

    int resolve(int font_size, ResolutionInfo, std::optional<int> percent_relative_to = std::nullopt) const;
};

struct UnresolvedLineHeight {
    UnresolvedValue line_height{};
    [[nodiscard]] bool operator==(UnresolvedLineHeight const &) const = default;

    [[nodiscard]] int resolve(
            int font_size, ResolutionInfo, std::optional<int> percent_relative_to = std::nullopt) const;
};

// NOLINTNEXTLINE(misc-no-recursion)
struct StyledNode {
    dom::Node const &node;
    std::vector<std::pair<css::PropertyId, std::string>> properties;
    std::vector<StyledNode> children;
    StyledNode const *parent{nullptr};
    std::vector<std::pair<std::string, std::string>> custom_properties;

    std::string_view get_raw_property(css::PropertyId) const;

    template<css::PropertyId T>
    auto get_property() const {
        if constexpr (T == css::PropertyId::BackgroundColor || T == css::PropertyId::BorderBottomColor
                || T == css::PropertyId::BorderLeftColor || T == css::PropertyId::BorderRightColor
                || T == css::PropertyId::BorderTopColor || T == css::PropertyId::OutlineColor
                || T == css::PropertyId::Color) {
            return get_color_property(T);
        } else if constexpr (T == css::PropertyId::BorderBottomStyle || T == css::PropertyId::BorderLeftStyle
                || T == css::PropertyId::BorderRightStyle || T == css::PropertyId::BorderTopStyle
                || T == css::PropertyId::OutlineStyle) {
            return get_border_style_property(T);
        } else if constexpr (T == css::PropertyId::Display) {
            return get_display_property();
        } else if constexpr (T == css::PropertyId::Float) {
            return get_float_property();
        } else if constexpr (T == css::PropertyId::FontFamily) {
            auto raw_font_family = get_raw_property(T);
            auto families = util::split(raw_font_family, ",");
            static constexpr auto kShouldTrim = [](char c) {
                return util::is_whitespace(c) || c == '\'' || c == '"';
            };
            std::ranges::for_each(families, [](auto &family) { family = util::trim(family, kShouldTrim); });
            return families;
        } else if constexpr (T == css::PropertyId::FontSize) {
            return get_font_size_property();
        } else if constexpr (T == css::PropertyId::FontStyle) {
            return get_font_style_property();
        } else if constexpr (T == css::PropertyId::FontWeight) {
            return get_font_weight_property();
        } else if constexpr (T == css::PropertyId::TextAlign) {
            return get_text_align_property();
        } else if constexpr (T == css::PropertyId::TextDecorationLine) {
            return get_text_decoration_line_property();
        } else if constexpr (T == css::PropertyId::TextTransform) {
            return get_text_transform_property();
        } else if constexpr (T == css::PropertyId::WhiteSpace) {
            return get_white_space_property();
        } else if constexpr (T == css::PropertyId::BorderBottomLeftRadius
                || T == css::PropertyId::BorderBottomRightRadius || T == css::PropertyId::BorderTopLeftRadius
                || T == css::PropertyId::BorderTopRightRadius) {
            return get_border_radius_property(T);
        } else if constexpr (T == css::PropertyId::MinWidth || T == css::PropertyId::Width
                || T == css::PropertyId::MaxWidth || T == css::PropertyId::MarginLeft
                || T == css::PropertyId::MarginRight || T == css::PropertyId::MarginTop
                || T == css::PropertyId::MarginBottom || T == css::PropertyId::PaddingLeft
                || T == css::PropertyId::PaddingRight || T == css::PropertyId::PaddingTop
                || T == css::PropertyId::PaddingBottom || T == css::PropertyId::MinHeight
                || T == css::PropertyId::Height || T == css::PropertyId::MaxHeight) {
            return UnresolvedValue{get_raw_property(T)};
        } else if constexpr (T == css::PropertyId::BorderBottomWidth || T == css::PropertyId::BorderLeftWidth
                || T == css::PropertyId::BorderRightWidth || T == css::PropertyId::BorderTopWidth) {
            return UnresolvedBorderWidth{{get_raw_property(T)}};
        } else if constexpr (T == css::PropertyId::LineHeight) {
            return UnresolvedLineHeight{{get_raw_property(T)}};
        } else {
            return get_raw_property(T);
        }
    }

private:
    std::optional<std::string_view> resolve_variable(std::string_view) const;

    BorderStyle get_border_style_property(css::PropertyId) const;
    gfx::Color get_color_property(css::PropertyId) const;
    std::optional<Display> get_display_property() const;
    std::optional<Float> get_float_property() const;
    FontStyle get_font_style_property() const;
    int get_font_size_property() const;
    std::optional<FontWeight> get_font_weight_property() const;
    TextAlign get_text_align_property() const;
    std::vector<TextDecorationLine> get_text_decoration_line_property() const;
    std::optional<TextTransform> get_text_transform_property() const;
    std::optional<WhiteSpace> get_white_space_property() const;
    std::pair<int, int> get_border_radius_property(css::PropertyId) const;
};

// NOLINTBEGIN(misc-no-recursion)
[[nodiscard]] inline bool operator==(style::StyledNode const &a, style::StyledNode const &b) noexcept {
    return a.node == b.node && a.properties == b.properties && a.custom_properties == b.custom_properties
            && a.children == b.children;
}
// NOLINTEND(misc-no-recursion)

inline std::string_view dom_name(StyledNode const &node) {
    return std::get<dom::Element>(node.node).name;
}

inline std::vector<StyledNode const *> dom_children(StyledNode const &node) {
    std::vector<StyledNode const *> children{};
    for (auto const &child : node.children) {
        if (!std::holds_alternative<dom::Element>(child.node)) {
            continue;
        }

        children.push_back(&child);
    }
    return children;
}

template<css::PropertyId T>
inline auto initial_value() {
    // Sad roundabout way of getting css::initial_value(T) parsed via the
    // StyledNode property system.
    // TODO(robinlinden): Don't require dummy nodes for this.
    dom::Node dom{};
    style::StyledNode style{.node = dom};
    return style.get_property<T>();
}

} // namespace style

#endif
