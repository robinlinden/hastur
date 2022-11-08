// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
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
    BorderTopStyle,
    BorderTopWidth,
    CaptionSide,
    Color,
    Cursor,
    Direction,
    Display,
    Elevation,
    EmptyCells,
    Font,
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
    TextIndent,
    TextTransform,
    Visibility,
    VoiceFamily,
    Volume,
    Widows,
    Width,
    WordSpacing, // When adding an id after this, remember to update the property id -> string test.
};

PropertyId property_id_from_string(std::string_view);

std::string_view to_string(PropertyId);

} // namespace css

#endif
