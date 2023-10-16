// SPDX-FileCopyrightText: 2022-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "css/property_id.h"

#include <algorithm>
#include <map>
#include <string_view>

using namespace std::literals;

namespace css {
namespace {

// NOLINTNEXTLINE(cert-err58-cpp)
std::map<std::string_view, PropertyId> const known_properties{
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
};

} // namespace

PropertyId property_id_from_string(std::string_view id) {
    if (!known_properties.contains(id)) {
        return PropertyId::Unknown;
    }

    return known_properties.at(id);
}

std::string_view to_string(PropertyId id) {
    auto it = std::ranges::find_if(known_properties, [id](auto const &entry) { return entry.second == id; });
    if (it != end(known_properties)) {
        return it->first;
    }

    return "unknown"sv;
}

} // namespace css
