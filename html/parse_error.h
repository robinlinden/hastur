// SPDX-FileCopyrightText: 2021-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef HTML_PARSE_ERROR_H_
#define HTML_PARSE_ERROR_H_

#include <cstdint>
#include <string_view>

namespace html {

enum class ParseError : std::uint8_t {
    AbruptClosingOfEmptyComment,
    AbruptDoctypePublicIdentifier,
    AbruptDoctypeSystemIdentifier,
    AbsenceOfDigitsInNumericCharacterReference,
    CdataInHtmlContent,
    CharacterReferenceOutsideUnicodeRange,
    ControlCharacterReference,
    DuplicateAttribute,
    EndTagWithAttributes,
    EndTagWithTrailingSolidus,
    EofBeforeTagName,
    EofInCdata,
    EofInComment,
    EofInDoctype,
    EofInScriptHtmlCommentLikeText,
    EofInTag,
    IncorrectlyClosedComment,
    IncorrectlyOpenedComment,
    InvalidCharacterSequenceAfterDoctypeName,
    InvalidFirstCharacterOfTagName,
    MissingAttributeValue,
    MissingDoctypeName,
    MissingDoctypePublicIdentifier,
    MissingDoctypeSystemIdentifier,
    MissingEndTagName,
    MissingQuoteBeforeDoctypePublicIdentifier,
    MissingQuoteBeforeDoctypeSystemIdentifier,
    MissingSemicolonAfterCharacterReference,
    MissingWhitespaceAfterDoctypePublicKeyword,
    MissingWhitespaceAfterDoctypeSystemKeyword,
    MissingWhitespaceBeforeDoctypeName,
    MissingWhitespaceBetweenAttributes,
    MissingWhitespaceBetweenDoctypePublicAndSystemIdentifiers,
    NestedComment,
    NoncharacterCharacterReference,
    NullCharacterReference,
    SurrogateCharacterReference,
    UnexpectedCharacterAfterDoctypeSystemIdentifier,
    UnexpectedCharacterInAttributeName,
    UnexpectedCharacterInUnquotedAttributeValue,
    UnexpectedEqualsSignBeforeAttributeName,
    UnexpectedNullCharacter,
    UnexpectedQuestionMarkInsteadOfTagName,
    UnexpectedSolidusInTag,
    UnknownNamedCharacterReference,
};

constexpr std::string_view to_string(ParseError e) {
    switch (e) {
        case ParseError::AbruptClosingOfEmptyComment:
            return "AbruptClosingOfEmptyComment";
        case ParseError::AbruptDoctypePublicIdentifier:
            return "AbruptDoctypePublicIdentifier";
        case ParseError::AbruptDoctypeSystemIdentifier:
            return "AbruptDoctypeSystemIdentifier";
        case ParseError::AbsenceOfDigitsInNumericCharacterReference:
            return "AbsenceOfDigitsInNumericCharacterReference";
        case ParseError::CdataInHtmlContent:
            return "CdataInHtmlContent";
        case ParseError::CharacterReferenceOutsideUnicodeRange:
            return "CharacterReferenceOutsideUnicodeRange";
        case ParseError::ControlCharacterReference:
            return "ControlCharacterReference";
        case ParseError::DuplicateAttribute:
            return "DuplicateAttribute";
        case ParseError::EndTagWithAttributes:
            return "EndTagWithAttributes";
        case ParseError::EndTagWithTrailingSolidus:
            return "EndTagWithTrailingSolidus";
        case ParseError::EofBeforeTagName:
            return "EofBeforeTagName";
        case ParseError::EofInCdata:
            return "EofInCdata";
        case ParseError::EofInComment:
            return "EofInComment";
        case ParseError::EofInDoctype:
            return "EofInDoctype";
        case ParseError::EofInScriptHtmlCommentLikeText:
            return "EofInScriptHtmlCommentLikeText";
        case ParseError::EofInTag:
            return "EofInTag";
        case ParseError::IncorrectlyClosedComment:
            return "IncorrectlyClosedComment";
        case ParseError::IncorrectlyOpenedComment:
            return "IncorrectlyOpenedComment";
        case ParseError::InvalidCharacterSequenceAfterDoctypeName:
            return "InvalidCharacterSequenceAfterDoctypeName";
        case ParseError::InvalidFirstCharacterOfTagName:
            return "InvalidFirstCharacterOfTagName";
        case ParseError::MissingAttributeValue:
            return "MissingAttributeValue";
        case ParseError::MissingDoctypeName:
            return "MissingDoctypeName";
        case ParseError::MissingDoctypePublicIdentifier:
            return "MissingDoctypePublicIdentifier";
        case ParseError::MissingDoctypeSystemIdentifier:
            return "MissingDoctypeSystemIdentifier";
        case ParseError::MissingEndTagName:
            return "MissingEndTagName";
        case ParseError::MissingQuoteBeforeDoctypePublicIdentifier:
            return "MissingQuoteBeforeDoctypePublicIdentifier";
        case ParseError::MissingQuoteBeforeDoctypeSystemIdentifier:
            return "MissingQuoteBeforeDoctypeSystemIdentifier";
        case ParseError::MissingSemicolonAfterCharacterReference:
            return "MissingSemicolonAfterCharacterReference";
        case ParseError::MissingWhitespaceAfterDoctypePublicKeyword:
            return "MissingWhitespaceAfterDoctypePublicKeyword";
        case ParseError::MissingWhitespaceAfterDoctypeSystemKeyword:
            return "MissingWhitespaceAfterDoctypeSystemKeyword";
        case ParseError::MissingWhitespaceBeforeDoctypeName:
            return "MissingWhitespaceBeforeDoctypeName";
        case ParseError::MissingWhitespaceBetweenAttributes:
            return "MissingWhitespaceBetweenAttributes";
        case ParseError::MissingWhitespaceBetweenDoctypePublicAndSystemIdentifiers:
            return "MissingWhitespaceBetweenDoctypePublicAndSystemIdentifiers";
        case ParseError::NestedComment:
            return "NestedComment";
        case ParseError::NoncharacterCharacterReference:
            return "NoncharacterCharacterReference";
        case ParseError::NullCharacterReference:
            return "NullCharacterReference";
        case ParseError::SurrogateCharacterReference:
            return "SurrogateCharacterReference";
        case ParseError::UnexpectedCharacterAfterDoctypeSystemIdentifier:
            return "UnexpectedCharacterAfterDoctypeSystemIdentifier";
        case ParseError::UnexpectedCharacterInAttributeName:
            return "UnexpectedCharacterInAttributeName";
        case ParseError::UnexpectedCharacterInUnquotedAttributeValue:
            return "UnexpectedCharacterInUnquotedAttributeValue";
        case ParseError::UnexpectedEqualsSignBeforeAttributeName:
            return "UnexpectedEqualsSignBeforeAttributeName";
        case ParseError::UnexpectedNullCharacter:
            return "UnexpectedNullCharacter";
        case ParseError::UnexpectedQuestionMarkInsteadOfTagName:
            return "UnexpectedQuestionMarkInsteadOfTagName";
        case ParseError::UnexpectedSolidusInTag:
            return "UnexpectedSolidusInTag";
        case ParseError::UnknownNamedCharacterReference:
            return "UnknownNamedCharacterReference";
    }

    return "Unknown error";
}

} // namespace html

#endif
