// SPDX-FileCopyrightText: 2022-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef CSS_PROPERTY_ID_H_
#define CSS_PROPERTY_ID_H_

#include <string_view>

namespace css {

enum class PropertyId {
    Unknown,

    Azimuth,
    BackgroundAttachment,
    BackgroundClip,
    BackgroundColor,
    BackgroundImage,
    BackgroundOrigin,
    BackgroundPosition,
    BackgroundRepeat,
    BackgroundSize,
    BorderBottomColor,
    BorderBottomLeftRadius,
    BorderBottomRightRadius,
    BorderBottomStyle,
    BorderBottomWidth,
    BorderCollapse,
    BorderLeftColor,
    BorderLeftStyle,
    BorderLeftWidth,
    BorderRightColor,
    BorderRightStyle,
    BorderRightWidth,
    BorderSpacing,
    BorderTopColor,
    BorderTopLeftRadius,
    BorderTopRightRadius,
    BorderTopStyle,
    BorderTopWidth,
    CaptionSide,
    Color,
    Cursor,
    Direction,
    Display,
    Elevation,
    EmptyCells,
    FlexBasis,
    FlexDirection,
    FlexGrow,
    FlexShrink,
    FlexWrap,
    FontFamily,
    FontFeatureSettings,
    FontKerning,
    FontLanguageOverride,
    FontOpticalSizing,
    FontPalette,
    FontSize,
    FontSizeAdjust,
    FontStretch,
    FontStyle,
    FontVariant,
    FontVariantAlternatives,
    FontVariantCaps,
    FontVariantEastAsian,
    FontVariantLigatures,
    FontVariantNumeric,
    FontVariantPosition,
    FontVariationSettings,
    FontWeight,
    Height,
    LetterSpacing,
    LineHeight,
    ListStyle,
    ListStyleImage,
    ListStylePosition,
    ListStyleType,
    MarginBottom,
    MarginLeft,
    MarginRight,
    MarginTop,
    MaxHeight,
    MaxWidth,
    MinHeight,
    MinWidth,
    Orphans,
    PaddingBottom,
    PaddingLeft,
    PaddingRight,
    PaddingTop,
    Pitch,
    PitchRange,
    Quotes,
    Richness,
    Speak,
    SpeakHeader,
    SpeakNumeral,
    SpeakPunctuation,
    SpeechRate,
    Stress,
    TextAlign,
    TextDecorationColor,
    TextDecorationLine,
    TextDecorationStyle,
    TextIndent,
    TextTransform,
    Visibility,
    VoiceFamily,
    Volume,
    WhiteSpace,
    Widows,
    Width,
    WordSpacing, // When adding an id after this, remember to update the property id -> string test.
};

PropertyId property_id_from_string(std::string_view);

std::string_view to_string(PropertyId);

// https://www.w3.org/TR/CSS22/propidx.html
constexpr bool is_inherited(PropertyId id) {
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
        case css::PropertyId::WhiteSpace:
        case css::PropertyId::Widows:
        case css::PropertyId::WordSpacing:
            return true;
        default:
            return false;
    }
}

} // namespace css

#endif
