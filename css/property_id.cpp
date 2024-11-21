// SPDX-FileCopyrightText: 2022-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "css/property_id.h"

#include <algorithm>
#include <array>
#include <map>
#include <string_view>
#include <utility>

using namespace std::literals;

namespace css {
namespace {

constexpr auto kKnownProperties = std::to_array<std::pair<std::string_view, PropertyId>>({
        {"azimuth"sv, PropertyId::Azimuth},
        {"background-attachment"sv, PropertyId::BackgroundAttachment},
        {"background-clip"sv, PropertyId::BackgroundClip},
        {"background-color"sv, PropertyId::BackgroundColor},
        {"background-image"sv, PropertyId::BackgroundImage},
        {"background-origin"sv, PropertyId::BackgroundOrigin},
        {"background-position"sv, PropertyId::BackgroundPosition},
        {"background-repeat"sv, PropertyId::BackgroundRepeat},
        {"background-size"sv, PropertyId::BackgroundSize},
        {"border-bottom-color"sv, PropertyId::BorderBottomColor},
        {"border-bottom-left-radius"sv, PropertyId::BorderBottomLeftRadius},
        {"border-bottom-right-radius"sv, PropertyId::BorderBottomRightRadius},
        {"border-bottom-style"sv, PropertyId::BorderBottomStyle},
        {"border-bottom-width"sv, PropertyId::BorderBottomWidth},
        {"border-collapse"sv, PropertyId::BorderCollapse},
        {"border-left-color"sv, PropertyId::BorderLeftColor},
        {"border-left-style"sv, PropertyId::BorderLeftStyle},
        {"border-left-width"sv, PropertyId::BorderLeftWidth},
        {"border-right-color"sv, PropertyId::BorderRightColor},
        {"border-right-style"sv, PropertyId::BorderRightStyle},
        {"border-right-width"sv, PropertyId::BorderRightWidth},
        {"border-spacing"sv, PropertyId::BorderSpacing},
        {"border-top-color"sv, PropertyId::BorderTopColor},
        {"border-top-left-radius"sv, PropertyId::BorderTopLeftRadius},
        {"border-top-right-radius"sv, PropertyId::BorderTopRightRadius},
        {"border-top-style"sv, PropertyId::BorderTopStyle},
        {"border-top-width"sv, PropertyId::BorderTopWidth},
        {"caption-side"sv, PropertyId::CaptionSide},
        {"color"sv, PropertyId::Color},
        {"color-scheme"sv, PropertyId::ColorScheme},
        {"cursor"sv, PropertyId::Cursor},
        {"direction"sv, PropertyId::Direction},
        {"display"sv, PropertyId::Display},
        {"elevation"sv, PropertyId::Elevation},
        {"empty-cells"sv, PropertyId::EmptyCells},
        {"flex-basis"sv, PropertyId::FlexBasis},
        {"flex-direction"sv, PropertyId::FlexDirection},
        {"flex-grow"sv, PropertyId::FlexGrow},
        {"flex-shrink"sv, PropertyId::FlexShrink},
        {"flex-wrap"sv, PropertyId::FlexWrap},
        {"float"sv, PropertyId::Float},
        {"font-family"sv, PropertyId::FontFamily},
        {"font-feature-settings"sv, PropertyId::FontFeatureSettings},
        {"font-kerning"sv, PropertyId::FontKerning},
        {"font-language-override"sv, PropertyId::FontLanguageOverride},
        {"font-optical-sizing"sv, PropertyId::FontOpticalSizing},
        {"font-palette"sv, PropertyId::FontPalette},
        {"font-size"sv, PropertyId::FontSize},
        {"font-size-adjust"sv, PropertyId::FontSizeAdjust},
        {"font-stretch"sv, PropertyId::FontStretch},
        {"font-style"sv, PropertyId::FontStyle},
        {"font-variant"sv, PropertyId::FontVariant},
        {"font-variant-alternatives"sv, PropertyId::FontVariantAlternatives},
        {"font-variant-caps"sv, PropertyId::FontVariantCaps},
        {"font-variant-east-asian"sv, PropertyId::FontVariantEastAsian},
        {"font-variant-ligatures"sv, PropertyId::FontVariantLigatures},
        {"font-variant-numeric"sv, PropertyId::FontVariantNumeric},
        {"font-variant-position"sv, PropertyId::FontVariantPosition},
        {"font-variation-settings"sv, PropertyId::FontVariationSettings},
        {"font-weight"sv, PropertyId::FontWeight},
        {"height"sv, PropertyId::Height},
        {"letter-spacing"sv, PropertyId::LetterSpacing},
        {"line-height"sv, PropertyId::LineHeight},
        {"list-style"sv, PropertyId::ListStyle},
        {"list-style-image"sv, PropertyId::ListStyleImage},
        {"list-style-position"sv, PropertyId::ListStylePosition},
        {"list-style-type"sv, PropertyId::ListStyleType},
        {"margin-bottom"sv, PropertyId::MarginBottom},
        {"margin-left"sv, PropertyId::MarginLeft},
        {"margin-right"sv, PropertyId::MarginRight},
        {"margin-top"sv, PropertyId::MarginTop},
        {"max-height"sv, PropertyId::MaxHeight},
        {"max-width"sv, PropertyId::MaxWidth},
        {"min-height"sv, PropertyId::MinHeight},
        {"min-width"sv, PropertyId::MinWidth},
        {"orphans"sv, PropertyId::Orphans},
        {"outline-color", PropertyId::OutlineColor},
        {"outline-style", PropertyId::OutlineStyle},
        {"outline-width", PropertyId::OutlineWidth},
        {"padding-bottom"sv, PropertyId::PaddingBottom},
        {"padding-left"sv, PropertyId::PaddingLeft},
        {"padding-right"sv, PropertyId::PaddingRight},
        {"padding-top"sv, PropertyId::PaddingTop},
        {"pitch"sv, PropertyId::Pitch},
        {"pitch-range"sv, PropertyId::PitchRange},
        {"quotes"sv, PropertyId::Quotes},
        {"richness"sv, PropertyId::Richness},
        {"speak"sv, PropertyId::Speak},
        {"speak-header"sv, PropertyId::SpeakHeader},
        {"speak-numeral"sv, PropertyId::SpeakNumeral},
        {"speak-punctuation"sv, PropertyId::SpeakPunctuation},
        {"speech-rate"sv, PropertyId::SpeechRate},
        {"stress"sv, PropertyId::Stress},
        {"text-align"sv, PropertyId::TextAlign},
        {"text-decoration-color"sv, PropertyId::TextDecorationColor},
        {"text-decoration-line"sv, PropertyId::TextDecorationLine},
        {"text-decoration-style"sv, PropertyId::TextDecorationStyle},
        {"text-indent"sv, PropertyId::TextIndent},
        {"text-transform"sv, PropertyId::TextTransform},
        {"visibility"sv, PropertyId::Visibility},
        {"voice-family"sv, PropertyId::VoiceFamily},
        {"volume"sv, PropertyId::Volume},
        {"white-space"sv, PropertyId::WhiteSpace},
        {"widows"sv, PropertyId::Widows},
        {"width"sv, PropertyId::Width},
        {"word-spacing"sv, PropertyId::WordSpacing},
});

// https://www.w3.org/TR/css-cascade/#initial-values
constexpr auto kInitialValues = std::to_array<std::pair<css::PropertyId, std::string_view>>({
        // https://developer.mozilla.org/en-US/docs/Web/CSS/background-color#formal_definition
        {css::PropertyId::BackgroundColor, "transparent"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/color#formal_definition
        {css::PropertyId::Color, "canvastext"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/color-scheme#formal_definition
        {css::PropertyId::ColorScheme, "normal"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/flex-basis#formal_definition
        {css::PropertyId::FlexBasis, "auto"sv},
        // https://developer.mozilla.org/en-US/docs/Web/CSS/flex-direction#formal_definition
        {css::PropertyId::FlexDirection, "row"sv},
        // https://developer.mozilla.org/en-US/docs/Web/CSS/flex-grow#formal_definition
        {css::PropertyId::FlexGrow, "0"sv},
        // https://developer.mozilla.org/en-US/docs/Web/CSS/flex-shrink#formal_definition
        {css::PropertyId::FlexShrink, "1"sv},
        // https://developer.mozilla.org/en-US/docs/Web/CSS/flex-wrap#formal_definition
        {css::PropertyId::FlexWrap, "nowrap"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/font-size#formal_definition
        {css::PropertyId::FontSize, "medium"sv},
        // https://developer.mozilla.org/en-US/docs/Web/CSS/font-family#formal_definition
        {css::PropertyId::FontFamily, "sans-serif"sv},
        // https://developer.mozilla.org/en-US/docs/Web/CSS/font-style#formal_definition
        {css::PropertyId::FontStyle, "normal"sv},
        // https://developer.mozilla.org/en-US/docs/Web/CSS/font-weight#formal_definition
        {css::PropertyId::FontWeight, "normal"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/text-decoration
        {css::PropertyId::TextDecorationColor, "currentcolor"sv},
        {css::PropertyId::TextDecorationLine, "none"sv},
        {css::PropertyId::TextDecorationStyle, "solid"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/text-transform#formal_definition
        {css::PropertyId::TextTransform, "none"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/border-color#formal_definition
        {css::PropertyId::BorderBottomColor, "currentcolor"sv},
        {css::PropertyId::BorderLeftColor, "currentcolor"sv},
        {css::PropertyId::BorderRightColor, "currentcolor"sv},
        {css::PropertyId::BorderTopColor, "currentcolor"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/border-radius
        {css::PropertyId::BorderBottomLeftRadius, "0"sv},
        {css::PropertyId::BorderBottomRightRadius, "0"sv},
        {css::PropertyId::BorderTopLeftRadius, "0"sv},
        {css::PropertyId::BorderTopRightRadius, "0"sv},

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

        // https://developer.mozilla.org/en-US/docs/Web/CSS/outline
        {css::PropertyId::OutlineColor, "currentcolor"},
        {css::PropertyId::OutlineStyle, "none"},
        {css::PropertyId::OutlineWidth, "medium"},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/padding#formal_definition
        {css::PropertyId::PaddingBottom, "0"sv},
        {css::PropertyId::PaddingLeft, "0"sv},
        {css::PropertyId::PaddingRight, "0"sv},
        {css::PropertyId::PaddingTop, "0"sv},

        // https://developer.mozilla.org/en-US/docs/Web/CSS/display#formal_definition
        {css::PropertyId::Display, "inline"sv},
        // https://developer.mozilla.org/en-US/docs/Web/CSS/float#formal_definition
        {css::PropertyId::Float, "none"sv},

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

        // https://developer.mozilla.org/en-US/docs/Web/CSS/white-space
        {css::PropertyId::WhiteSpace, "normal"sv},
});

} // namespace

PropertyId property_id_from_string(std::string_view id) {
    // NOLINTNEXTLINE(readability-qualified-auto): Not guaranteed to be a ptr.
    if (auto it = std::ranges::find(kKnownProperties, id, &decltype(kKnownProperties)::value_type::first);
            it != end(kKnownProperties)) {
        return it->second;
    }

    return PropertyId::Unknown;
}

std::string_view to_string(PropertyId id) {
    // NOLINTNEXTLINE(readability-qualified-auto): Not guaranteed to be a ptr.
    auto it = std::ranges::find_if(kKnownProperties, [id](auto const &entry) { return entry.second == id; });
    if (it != end(kKnownProperties)) {
        return it->first;
    }

    return "unknown"sv;
}

std::string_view initial_value(PropertyId id) {
    return std::ranges::find(kInitialValues, id, &decltype(kInitialValues)::value_type::first)->second;
}

} // namespace css
