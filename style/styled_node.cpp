// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "style/styled_node.h"

#include <algorithm>
#include <string_view>

using namespace std::literals;

namespace style {
namespace {

// https://www.w3.org/TR/CSS22/propidx.html
constexpr bool is_inherited(css::PropertyId id) {
    switch (id) {
        case css::PropertyId::Azimuth:
        case css::PropertyId::BorderCollapse:
        case css::PropertyId::BorderSpacing:
        case css::PropertyId::CaptionSide:
        case css::PropertyId::Color:
        case css::PropertyId::Cursor:
        case css::PropertyId::Direction:
        case css::PropertyId::Elevation:
        case css::PropertyId::EmptyCells:
        case css::PropertyId::Font:
        case css::PropertyId::FontFamily:
        case css::PropertyId::FontSize:
        case css::PropertyId::FontStyle:
        case css::PropertyId::FontVariant:
        case css::PropertyId::FontWeight:
        case css::PropertyId::LetterSpacing:
        case css::PropertyId::LineHeight:
        case css::PropertyId::ListStyle:
        case css::PropertyId::ListStyleImage:
        case css::PropertyId::ListStylePosition:
        case css::PropertyId::ListStyleType:
        case css::PropertyId::Orphans:
        case css::PropertyId::Pitch:
        case css::PropertyId::PitchRange:
        case css::PropertyId::Quotes:
        case css::PropertyId::Richness:
        case css::PropertyId::Speak:
        case css::PropertyId::SpeakHeader:
        case css::PropertyId::SpeakNumeral:
        case css::PropertyId::SpeakPunctuation:
        case css::PropertyId::SpeechRate:
        case css::PropertyId::Stress:
        case css::PropertyId::TextAlign:
        case css::PropertyId::TextIndent:
        case css::PropertyId::TextTransform:
        case css::PropertyId::Visibility:
        case css::PropertyId::VoiceFamily:
        case css::PropertyId::Volume:
        case css::PropertyId::Widows:
        case css::PropertyId::WordSpacing:
            return true;
        default:
            return false;
    }
}

// https://www.w3.org/TR/css-cascade/#initial-values
std::map<css::PropertyId, std::string_view> const kInitialValues{
        // https://developer.mozilla.org/en-US/docs/Web/CSS/background-color#formal_definition
        {css::PropertyId::BackgroundColor, "transparent"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/color#formal_definition
        {css::PropertyId::Color, "canvastext"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/font-size#formal_definition
        {css::PropertyId::FontSize, "medium"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/border-color#formal_definition
        {css::PropertyId::BorderBottomColor, "currentcolor"sv},
        {css::PropertyId::BorderLeftColor, "currentcolor"sv},
        {css::PropertyId::BorderRightColor, "currentcolor"sv},
        {css::PropertyId::BorderTopColor, "currentcolor"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/border-style#formal_definition
        {css::PropertyId::BorderBottomStyle, "none"sv},
        {css::PropertyId::BorderLeftStyle, "none"sv},
        {css::PropertyId::BorderRightStyle, "none"sv},
        {css::PropertyId::BorderTopStyle, "none"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/border-width#formal_definition
        {css::PropertyId::BorderBottomWidth, "medium"sv},
        {css::PropertyId::BorderLeftWidth, "medium"sv},
        {css::PropertyId::BorderRightWidth, "medium"sv},
        {css::PropertyId::BorderTopWidth, "medium"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/padding#formal_definition
        {css::PropertyId::PaddingBottom, "0"sv},
        {css::PropertyId::PaddingLeft, "0"sv},
        {css::PropertyId::PaddingRight, "0"sv},
        {css::PropertyId::PaddingTop, "0"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/display#formal_definition
        {css::PropertyId::Display, "inline"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/height#formal_definition
        // https://developer.mozilla.org/en-US/docs/Web/CSS/max-height#formal_definition
        // https://developer.mozilla.org/en-US/docs/Web/CSS/min-height#formal_definition
        {css::PropertyId::Height, "auto"sv},
        {css::PropertyId::MaxHeight, "none"sv},
        {css::PropertyId::MinHeight, "auto"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/margin#formal_definition
        {css::PropertyId::MarginBottom, "0"sv},
        {css::PropertyId::MarginLeft, "0"sv},
        {css::PropertyId::MarginRight, "0"sv},
        {css::PropertyId::MarginTop, "0"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/width#formal_definition
        // https://developer.mozilla.org/en-US/docs/Web/CSS/max-width#formal_definition
        // https://developer.mozilla.org/en-US/docs/Web/CSS/min-width#formal_definition
        {css::PropertyId::Width, "auto"sv},
        {css::PropertyId::MaxWidth, "none"sv},
        {css::PropertyId::MinWidth, "auto"sv},
};

std::string_view get_parent_property(style::StyledNode const &node, css::PropertyId property) {
    if (node.parent != nullptr) {
        return node.parent->get_property(property);
    }

    return kInitialValues.at(property);
}

} // namespace

std::string_view StyledNode::get_property(css::PropertyId property) const {
    auto it = std::ranges::find_if(properties, [=](auto const &p) { return p.first == property; });

    if (it == cend(properties)) {
        if (is_inherited(property) && parent != nullptr) {
            return parent->get_property(property);
        }

        return kInitialValues.at(property);
    } else if (it->second == "initial") {
        // https://developer.mozilla.org/en-US/docs/Web/CSS/initial
        return kInitialValues.at(property);
    } else if (it->second == "inherit") {
        // https://developer.mozilla.org/en-US/docs/Web/CSS/inherit
        return get_parent_property(*this, property);
    } else if (it->second == "currentcolor") {
        // https://developer.mozilla.org/en-US/docs/Web/CSS/color_value#currentcolor_keyword
        // If the "color" property has the value "currentcolor", treat it as "inherit".
        if (it->first == css::PropertyId::Color) {
            return get_parent_property(*this, property);
        }

        // Even though we return the correct value here, if a property has
        // "currentcolor" as its initial value, the caller have to manually look
        // up the value of "color". This will be cleaned up along with the rest
        // of the property management soon.
        return get_property(css::PropertyId::Color);
    }

    return it->second;
}

} // namespace style
