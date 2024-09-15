// SPDX-FileCopyrightText: 2021-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "style/styled_node.h"

#include "style/unresolved_value.h"

#include "css/property_id.h"
#include "dom/dom.h"
#include "gfx/color.h"
#include "util/from_chars.h"
#include "util/string.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>
#include <vector>

namespace style {
namespace {

int get_root_font_size(style::StyledNode const &node) {
    auto const *n = &node;
    while (n->parent != nullptr) {
        n = n->parent;
    }
    return n->get_property<css::PropertyId::FontSize>();
}

std::optional<gfx::Color> try_from_hex_chars(std::string_view hex_chars) {
    if (!hex_chars.starts_with('#')) {
        return std::nullopt;
    }

    hex_chars.remove_prefix(1);
    std::uint32_t hex{};
    if (hex_chars.length() == 6) {
        std::from_chars(hex_chars.data(), hex_chars.data() + hex_chars.size(), hex, /*base*/ 16);
        return gfx::Color::from_rgb(hex);
    }

    if (hex_chars.length() == 3) {
        std::ostringstream ss;
        ss << hex_chars[0] << hex_chars[0] << hex_chars[1] << hex_chars[1] << hex_chars[2] << hex_chars[2];
        auto expanded = std::move(ss).str();
        std::from_chars(expanded.data(), expanded.data() + expanded.size(), hex, /*base*/ 16);
        return gfx::Color::from_rgb(hex);
    }

    if (hex_chars.length() == 8) {
        std::from_chars(hex_chars.data(), hex_chars.data() + hex_chars.size(), hex, /*base*/ 16);
        return gfx::Color::from_rgba(hex);
    }

    if (hex_chars.length() == 4) {
        std::ostringstream ss;
        ss << hex_chars[0] << hex_chars[0] << hex_chars[1] << hex_chars[1] << hex_chars[2] << hex_chars[2]
           << hex_chars[3] << hex_chars[3];
        auto expanded = std::move(ss).str();
        std::from_chars(expanded.data(), expanded.data() + expanded.size(), hex, /*base*/ 16);
        return gfx::Color::from_rgba(hex);
    }

    return std::nullopt;
}

// https://developer.mozilla.org/en-US/docs/Web/CSS/color_value/rgb
std::optional<gfx::Color> try_from_rgba(std::string_view text) {
    if (text.starts_with("rgb(")) {
        text.remove_prefix(std::strlen("rgb("));
    } else if (text.starts_with("rgba(")) {
        text.remove_prefix(std::strlen("rgba("));
    } else {
        return std::nullopt;
    }

    if (!text.ends_with(')')) {
        return std::nullopt;
    }
    text.remove_suffix(std::strlen(")"));

    // First try to handle rgba(1, 2, 3, .5)
    auto rgba = util::split(text, ",");
    if (rgba.size() == 1) {
        // And then rgba(1 2 3 / .5)
        rgba = util::split(text, "/");
        if (rgba.size() == 2) {
            auto a = rgba[1];
            rgba = util::split(rgba[0], " ");
            rgba.push_back(a);
        } else {
            rgba = util::split(text, " ");
        }

        // Nuke any empty segments. This happens if you have more than 1 space
        // between the rgba arguments.
        std::erase_if(rgba, [](auto const &s) { return empty(s); });
    }

    if (rgba.size() != 3 && rgba.size() != 4) {
        return std::nullopt;
    }

    for (auto &value : rgba) {
        value = util::trim(value);
    }

    auto to_int = [](std::string_view v) {
        int ret{-1};
        if (std::from_chars(v.data(), v.data() + v.size(), ret).ptr != v.data() + v.size()) {
            return -1;
        }

        if (ret < 0 || ret > 255) {
            return -1;
        }

        return ret;
    };

    auto r{to_int(rgba[0])};
    auto g{to_int(rgba[1])};
    auto b{to_int(rgba[2])};
    if (r == -1 || g == -1 || b == -1) {
        return std::nullopt;
    }

    if (rgba.size() == 3) {
        return gfx::Color{static_cast<std::uint8_t>(r), static_cast<std::uint8_t>(g), static_cast<std::uint8_t>(b)};
    }

    float a{-1.f};
    if (util::from_chars(rgba[3].data(), rgba[3].data() + rgba[3].size(), a).ptr != rgba[3].data() + rgba[3].size()) {
        return std::nullopt;
    }

    a = std::clamp(a, 0.f, 1.f);

    return gfx::Color{
            static_cast<std::uint8_t>(r),
            static_cast<std::uint8_t>(g),
            static_cast<std::uint8_t>(b),
            static_cast<std::uint8_t>(a * 255),
    };
}

gfx::Color parse_color(std::string_view str) {
    if (auto color = try_from_hex_chars(str)) {
        return *color;
    }

    if (auto color = try_from_rgba(str)) {
        return *color;
    }

    if (auto css_named_color = gfx::Color::from_css_name(str)) {
        return *css_named_color;
    }

    spdlog::warn("Unrecognized color format: {}", str);
    return gfx::Color{0xFF, 0, 0};
}

// NOLINTNEXTLINE(misc-no-recursion)
std::string_view get_parent_raw_property(style::StyledNode const &node, css::PropertyId property) {
    if (node.parent != nullptr) {
        return node.parent->get_raw_property(property);
    }

    return css::initial_value(property);
}

std::optional<std::pair<float, std::string_view>> split_into_value_and_unit(std::string_view property) {
    float res{};
    auto parse_result = util::from_chars(property.data(), property.data() + property.size(), res);
    if (parse_result.ec != std::errc{}) {
        spdlog::warn("Unable to split '{}' in split_into_value_and_unit", property);
        return std::nullopt;
    }

    auto const parsed_length = std::distance(property.data(), parse_result.ptr);
    auto const unit = property.substr(parsed_length);
    return std::pair{res, unit};
}

// https://drafts.csswg.org/css-backgrounds/#the-border-width
constexpr auto kBorderWidthKeywords = std::to_array<std::pair<std::string_view, int>>({
        {"thin", 3},
        {"medium", 5},
        {"thick", 7},
});

} // namespace

int UnresolvedBorderWidth::resolve(
        int font_size, ResolutionInfo context, std::optional<int> percent_relative_to) const {
    // NOLINTNEXTLINE(readability-qualified-auto): Not guaranteed to be a ptr.
    if (auto it = std::ranges::find(
                kBorderWidthKeywords, width.raw, &decltype(kBorderWidthKeywords)::value_type::first);
            it != kBorderWidthKeywords.end()) {
        return it->second;
    }

    return width.resolve(font_size, context, percent_relative_to);
}

// NOLINTNEXTLINE(misc-no-recursion)
std::string_view StyledNode::get_raw_property(css::PropertyId property) const {
    // We don't support selector specificity yet, so the last property is found
    // in order to allow website style to override the browser built-in style.
    auto it = std::ranges::find_if(
            rbegin(properties), rend(properties), [=](auto const &p) { return p.first == property; });

    // TODO(robinlinden): Having a special case for dom::Text here doesn't feel good.
    // You can't set properties on text nodes in HTML (even though we do in
    // tests), so let's grab this from the parent node.
    if (it == rend(properties) && std::holds_alternative<dom::Text>(node) && parent != nullptr) {
        return parent->get_raw_property(property);
    }

    if (it == rend(properties) || it->second == "unset") {
        // https://developer.mozilla.org/en-US/docs/Web/CSS/unset
        if (is_inherited(property) && parent != nullptr) {
            return parent->get_raw_property(property);
        }

        return css::initial_value(property);
    }

    if (it->second == "initial") {
        // https://developer.mozilla.org/en-US/docs/Web/CSS/initial
        return css::initial_value(property);
    }

    if (it->second == "inherit") {
        // https://developer.mozilla.org/en-US/docs/Web/CSS/inherit
        return get_parent_raw_property(*this, property);
    }

    if (it->second == "currentcolor") {
        // https://developer.mozilla.org/en-US/docs/Web/CSS/color_value#currentcolor_keyword
        // If the "color" property has the value "currentcolor", treat it as "inherit".
        if (it->first == css::PropertyId::Color) {
            return get_parent_raw_property(*this, property);
        }

        // Even though we return the correct value here, if a property has
        // "currentcolor" as its initial value, the caller have to manually look
        // up the value of "color". This will be cleaned up along with the rest
        // of the property management soon.
        return get_raw_property(css::PropertyId::Color);
    }

    // If this is a var() we can easily expand here, do so.
    if (it->second.starts_with("var(") && (it->second.find(')') != std::string::npos)) {
        auto value = std::string_view{it->second};

        // Remove "var(" from the start and ")" from the end. 5 characters in total.
        auto var = value.substr(4, value.size() - 5);
        auto [var_name, fallback] = util::split_once(var, ",");
        auto prop = resolve_variable(var_name);
        if (!prop) {
            fallback = util::trim(fallback);
            if (!fallback.empty()) {
                return fallback;
            }

            return it->second;
        }

        return *prop;
    }

    return it->second;
}

// NOLINTNEXTLINE(misc-no-recursion)
std::optional<std::string_view> StyledNode::resolve_variable(std::string_view name) const {
    auto prop = std::ranges::find(custom_properties, name, &std::pair<std::string, std::string>::first);
    if (prop == end(custom_properties)) {
        if (parent != nullptr) {
            return parent->resolve_variable(name);
        }

        spdlog::info("No matching variable for custom property '{}'", name);
        return std::nullopt;
    }

    return prop->second;
}

BorderStyle StyledNode::get_border_style_property(css::PropertyId property) const {
    auto raw = get_raw_property(property);

    if (raw == "none") {
        return BorderStyle::None;
    }

    if (raw == "hidden") {
        return BorderStyle::Hidden;
    }

    if (raw == "dotted") {
        return BorderStyle::Dotted;
    }

    if (raw == "dashed") {
        return BorderStyle::Dashed;
    }

    if (raw == "solid") {
        return BorderStyle::Solid;
    }

    if (raw == "double") {
        return BorderStyle::Double;
    }

    if (raw == "groove") {
        return BorderStyle::Groove;
    }

    if (raw == "ridge") {
        return BorderStyle::Ridge;
    }

    if (raw == "inset") {
        return BorderStyle::Inset;
    }

    if (raw == "outset") {
        return BorderStyle::Outset;
    }

    spdlog::warn("Unhandled border-style value '{}'", raw);
    return BorderStyle::None;
}

gfx::Color StyledNode::get_color_property(css::PropertyId property) const {
    auto color_text = get_raw_property(property);

    // https://developer.mozilla.org/en-US/docs/Web/CSS/color_value#currentcolor_keyword
    if (color_text == "currentcolor") {
        color_text = get_raw_property(css::PropertyId::Color);
    }

    return parse_color(color_text);
}

// https://developer.mozilla.org/en-US/docs/Web/CSS/float
// ^ has info about the weird float<->display property interaction.
std::optional<Display> StyledNode::get_display_property() const {
    // TODO(robinlinden): Special-case for text not needed once the special case
    // where we get the parent properties for text in get_raw_property is
    // removed.
    if (std::holds_alternative<dom::Text>(node)) {
        return Display::inline_flow();
    }

    auto raw = get_raw_property(css::PropertyId::Display);
    if (raw == "none") {
        return std::nullopt;
    }

    if (raw == "inline") {
        if (get_property<css::PropertyId::Float>().value_or(Float::None) == Float::None) {
            return Display::inline_flow();
        }

        return Display::block_flow();
    }

    if (raw == "block") {
        return Display::block_flow();
    }

    spdlog::warn("Unhandled display value '{}'", raw);
    return Display::block_flow();
}

std::optional<Float> StyledNode::get_float_property() const {
    auto raw = get_raw_property(css::PropertyId::Float);
    if (raw == "none") {
        return Float::None;
    }

    if (raw == "left") {
        return Float::Left;
    }

    if (raw == "right") {
        return Float::Right;
    }

    if (raw == "inline-start") {
        return Float::InlineStart;
    }

    if (raw == "inline-end") {
        return Float::InlineEnd;
    }

    return std::nullopt;
}

FontStyle StyledNode::get_font_style_property() const {
    auto raw = get_raw_property(css::PropertyId::FontStyle);
    if (raw == "normal") {
        return FontStyle::Normal;
    }

    if (raw == "italic") {
        return FontStyle::Italic;
    }

    if (raw == "oblique") {
        return FontStyle::Oblique;
    }

    spdlog::warn("Unhandled font style value {}", raw);
    return FontStyle::Normal;
}

std::vector<TextDecorationLine> StyledNode::get_text_decoration_line_property() const {
    auto into = [](std::string_view v) -> std::optional<TextDecorationLine> {
        if (v == "none") {
            return TextDecorationLine::None;
        }

        if (v == "underline") {
            return TextDecorationLine::Underline;
        }

        if (v == "overline") {
            return TextDecorationLine::Overline;
        }

        if (v == "line-through") {
            return TextDecorationLine::LineThrough;
        }

        if (v == "blink") {
            return TextDecorationLine::Blink;
        }

        spdlog::warn("Unhandled text-decoration-line value '{}'", v);
        return std::nullopt;
    };

    std::vector<TextDecorationLine> lines;

    auto parts = util::split(get_raw_property(css::PropertyId::TextDecorationLine), " ");
    for (auto const &part : parts) {
        if (auto line = into(part)) {
            lines.push_back(*line);
        } else {
            return {};
        }
    }

    return lines;
}

std::optional<TextTransform> StyledNode::get_text_transform_property() const {
    auto raw = get_raw_property(css::PropertyId::TextTransform);
    if (raw == "none") {
        return TextTransform::None;
    }

    if (raw == "capitalize") {
        return TextTransform::Capitalize;
    }

    if (raw == "uppercase") {
        return TextTransform::Uppercase;
    }

    if (raw == "lowercase") {
        return TextTransform::Lowercase;
    }

    if (raw == "full-width") {
        return TextTransform::FullWidth;
    }

    if (raw == "full-size-kana") {
        return TextTransform::FullSizeKana;
    }

    spdlog::warn("Unhandled text-transform value '{}'", raw);
    return std::nullopt;
}

static constexpr int kDefaultFontSize{16};
// https://drafts.csswg.org/css-fonts-4/#absolute-size-mapping
constexpr int kMediumFontSize = kDefaultFontSize;
constexpr auto kFontSizeAbsoluteSizeKeywords = std::to_array<std::pair<std::string_view, float>>({
        {"xx-small", 3 / 5.f},
        {"x-small", 3 / 4.f},
        {"small", 8 / 9.f},
        {"medium", 1.f},
        {"large", 6 / 5.f},
        {"x-large", 3 / 2.f},
        {"xx-large", 2 / 1.f},
        {"xxx-large", 3 / 1.f},
});

// NOLINTNEXTLINE(misc-no-recursion)
int StyledNode::get_font_size_property() const {
    auto get_closest_font_size_and_owner =
            [](StyledNode const *starting_node) -> std::optional<std::pair<std::string_view, StyledNode const *>> {
        for (auto const *n = starting_node; n != nullptr; n = n->parent) {
            auto it = std::ranges::find_if(rbegin(n->properties), rend(n->properties), [](auto const &v) {
                return v.first == css::PropertyId::FontSize;
            });
            if (it != rend(n->properties) && it->second != "inherit" && it->second != "unset") {
                return {{it->second, n}};
            }
        }

        return std::nullopt;
    };

    auto closest = get_closest_font_size_and_owner(this);
    if (!closest) {
        return kDefaultFontSize;
    }
    auto raw_value = closest->first;

    // NOLINTNEXTLINE(readability-qualified-auto): Not guaranteed to be a ptr.
    if (auto it = std::ranges::find(
                kFontSizeAbsoluteSizeKeywords, raw_value, &decltype(kFontSizeAbsoluteSizeKeywords)::value_type::first);
            it != end(kFontSizeAbsoluteSizeKeywords)) {
        return std::lround(it->second * kMediumFontSize);
    }

    // NOLINTNEXTLINE(misc-no-recursion)
    auto parent_or_default_font_size = [&] {
        auto const *owner = closest->second;
        if (owner->parent == nullptr) {
            return kDefaultFontSize;
        }

        return owner->parent->get_font_size_property();
    };

    // https://drafts.csswg.org/css-fonts-4/#valdef-font-size-relative-size
    constexpr auto kRelativeFontSizeRatio = 1.2f;
    if (raw_value == "larger") {
        return static_cast<int>(parent_or_default_font_size() * kRelativeFontSizeRatio);
    }

    if (raw_value == "smaller") {
        return static_cast<int>(parent_or_default_font_size() / kRelativeFontSizeRatio);
    }

    auto value_and_unit = split_into_value_and_unit(raw_value);
    if (!value_and_unit) {
        return kDefaultFontSize;
    }
    auto [value, unit] = *value_and_unit;

    if (value == 0) {
        return 0;
    }

    if (unit == "px") {
        return static_cast<int>(value);
    }

    if (unit == "em") {
        return static_cast<int>(value * parent_or_default_font_size());
    }

    if (unit == "%") {
        return static_cast<int>(value / 100.f * parent_or_default_font_size());
    }

    if (unit == "rem") {
        auto const *root = [&] {
            auto const *n = closest->second;
            while (n->parent != nullptr) {
                n = n->parent;
            }
            return n;
        }();
        auto root_font_size = (root != nullptr) && root != this ? root->get_font_size_property() : kDefaultFontSize;
        return static_cast<int>(value * root_font_size);
    }

    if (unit == "pt") {
        // 12pt seems to generally equal 16px.
        static constexpr float kPtToPxRatio = 16.f / 12.f;
        return static_cast<int>(value * kPtToPxRatio);
    }

    // https://www.w3.org/TR/css3-values/#ex
    // https://www.w3.org/TR/css3-values/#ch
    if (unit == "ex" || unit == "ch") {
        // Technically, these are the height of an 'x' or '0' glyph
        // respectively, but we're allowed to approximate it as 50% of the em
        // value.
        static constexpr float kExToEmRatio = 0.5f;
        return static_cast<int>(value * kExToEmRatio * parent_or_default_font_size());
    }

    spdlog::warn("Unhandled unit '{}'", unit);
    return 0;
}

// https://drafts.csswg.org/css-fonts-4/#font-weight-prop
// NOLINTNEXTLINE(misc-no-recursion)
std::optional<FontWeight> StyledNode::get_font_weight_property() const {
    auto raw = get_raw_property(css::PropertyId::FontWeight);
    if (raw == "normal") {
        return FontWeight::normal();
    }

    if (raw == "bold") {
        return FontWeight::bold();
    }

    if (raw == "bolder") {
        // NOLINTNEXTLINE(misc-no-recursion)
        auto parent_weight = [&] {
            if (parent == nullptr) {
                return FontWeight::normal();
            }

            return parent->get_font_weight_property().value_or(FontWeight::normal());
        }();

        // https://drafts.csswg.org/css-fonts-4/#relative-weights
        if (parent_weight.value < 350) {
            return FontWeight::normal();
        }

        if (parent_weight.value < 550) {
            return FontWeight::bold();
        }

        if (parent_weight.value < 900) {
            return FontWeight{900};
        }

        return parent_weight;
    }

    if (raw == "lighter") {
        // NOLINTNEXTLINE(misc-no-recursion)
        auto parent_weight = [&] {
            if (parent == nullptr) {
                return FontWeight::normal();
            }

            return parent->get_font_weight_property().value_or(FontWeight::normal());
        }();

        // https://drafts.csswg.org/css-fonts-4/#relative-weights
        if (parent_weight.value < 100) {
            return parent_weight;
        }

        if (parent_weight.value < 550) {
            return FontWeight{100};
        }

        if (parent_weight.value < 750) {
            return FontWeight::normal();
        }

        return FontWeight::bold();
    }

    int weight{-1};
    if (auto res = std::from_chars(raw.data(), raw.data() + raw.size(), weight);
            res.ec != std::errc{} || res.ptr != raw.data() + raw.size()) {
        return std::nullopt;
    }

    if (weight < 1 || weight > 1000) {
        return std::nullopt;
    }

    return FontWeight{weight};
}

std::optional<WhiteSpace> StyledNode::get_white_space_property() const {
    auto raw = get_raw_property(css::PropertyId::WhiteSpace);
    if (raw == "normal") {
        return WhiteSpace::Normal;
    }

    if (raw == "pre") {
        return WhiteSpace::Pre;
    }

    if (raw == "nowrap") {
        return WhiteSpace::Nowrap;
    }

    if (raw == "pre-wrap") {
        return WhiteSpace::PreWrap;
    }

    if (raw == "break-spaces") {
        return WhiteSpace::BreakSpaces;
    }

    if (raw == "pre-line") {
        return WhiteSpace::PreLine;
    }

    spdlog::warn("Unhandled white-space '{}'", raw);
    return std::nullopt;
}

std::pair<int, int> StyledNode::get_border_radius_property(css::PropertyId id) const {
    auto raw = get_raw_property(id);
    auto [horizontal, vertical] = raw.contains('/') ? util::split_once(raw, "/") : std::pair{raw, raw};
    auto horizontal_prop = UnresolvedValue{horizontal};
    auto vertical_prop = UnresolvedValue{vertical};

    int font_size = get_property<css::PropertyId::FontSize>();
    int root_font_size = get_root_font_size(*this);
    return {
            horizontal_prop.resolve(font_size, {.root_font_size = root_font_size}),
            vertical_prop.resolve(font_size, {.root_font_size = root_font_size}),
    };
}

} // namespace style
