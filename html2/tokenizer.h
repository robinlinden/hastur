// SPDX-FileCopyrightText: 2021-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef HTML2_TOKENIZER_H_
#define HTML2_TOKENIZER_H_

#include "html2/token.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace html2 {

// https://html.spec.whatwg.org/multipage/parsing.html#tokenization
enum class State : std::uint8_t {
    Data = 1, // So the state enum values match the number in the spec.
    Rcdata,
    Rawtext,
    ScriptData,
    Plaintext,
    TagOpen,
    EndTagOpen,
    TagName,
    RcdataLessThanSign,
    RcdataEndTagOpen,
    RcdataEndTagName,
    RawtextLessThanSign,
    RawtextEndTagOpen,
    RawtextEndTagName,
    ScriptDataLessThanSign,
    ScriptDataEndTagOpen,
    ScriptDataEndTagName,
    ScriptDataEscapeStart,
    ScriptDataEscapeStartDash,
    ScriptDataEscaped,
    ScriptDataEscapedDash,
    ScriptDataEscapedDashDash,
    ScriptDataEscapedLessThanSign,
    ScriptDataEscapedEndTagOpen,
    ScriptDataEscapedEndTagName,
    ScriptDataDoubleEscapeStart,
    ScriptDataDoubleEscaped,
    ScriptDataDoubleEscapedDash,
    ScriptDataDoubleEscapedDashDash,
    ScriptDataDoubleEscapedLessThanSign,
    ScriptDataDoubleEscapeEnd,
    BeforeAttributeName,
    AttributeName,
    AfterAttributeName,
    BeforeAttributeValue,
    AttributeValueDoubleQuoted,
    AttributeValueSingleQuoted,
    AttributeValueUnquoted,
    AfterAttributeValueQuoted,
    SelfClosingStartTag,
    BogusComment,
    MarkupDeclarationOpen,
    CommentStart,
    CommentStartDash,
    Comment,
    CommentLessThanSign,
    CommentLessThanSignBang,
    CommentLessThanSignBangDash,
    CommentLessThanSignBangDashDash,
    CommentEndDash,
    CommentEnd,
    CommentEndBang,
    Doctype,
    BeforeDoctypeName,
    DoctypeName,
    AfterDoctypeName,
    AfterDoctypePublicKeyword,
    BeforeDoctypePublicIdentifier,
    DoctypePublicIdentifierDoubleQuoted,
    DoctypePublicIdentifierSingleQuoted,
    AfterDoctypePublicIdentifier,
    BetweenDoctypePublicAndSystemIdentifiers,
    AfterDoctypeSystemKeyword,
    BeforeDoctypeSystemIdentifier,
    DoctypeSystemIdentifierDoubleQuoted,
    DoctypeSystemIdentifierSingleQuoted,
    AfterDoctypeSystemIdentifier,
    BogusDoctype,
    CdataSection,
    CdataSectionBracket,
    CdataSectionEnd,
    CharacterReference,
    NamedCharacterReference,
    AmbiguousAmpersand,
    NumericCharacterReference,
    HexadecimalCharacterReferenceStart,
    DecimalCharacterReferenceStart,
    HexadecimalCharacterReference,
    DecimalCharacterReference,
    NumericCharacterReferenceEnd,
};

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

struct SourceLocation {
    int line{};
    int column{};
    [[nodiscard]] bool operator==(SourceLocation const &) const = default;
};

class Tokenizer {
public:
    Tokenizer(
            std::string_view input,
            std::function<void(Tokenizer &, Token &&)> on_emit,
            std::function<void(Tokenizer &, ParseError)> on_error = [](auto &, auto) {})
        : input_{input}, on_emit_{std::move(on_emit)}, on_error_{std::move(on_error)} {}

    void set_state(State);
    void run();

    [[nodiscard]] SourceLocation current_source_location() const;

    // This will definitely change once we implement the tree construction, but this works for now.
    // https://html.spec.whatwg.org/multipage/parsing.html#markup-declaration-open-state
    void set_adjusted_current_node_in_html_namespace(bool in_html_namespace) {
        adjusted_current_node_in_html_namespace_ = in_html_namespace;
    }

private:
    std::string_view input_;
    std::size_t pos_{0};
    State state_{State::Data};
    State return_state_{};
    Token current_token_{};

    std::string temporary_buffer_{};
    std::string last_start_tag_name_{};

    std::uint32_t character_reference_code_{};
    bool adjusted_current_node_in_html_namespace_{true};

    // These end-tag bits aren't allowed to leave the tokenizer, but we need to
    // keep them around internally to emit warnings when reasonable.
    bool self_closing_end_tag_detected_{false};
    std::vector<Attribute> end_tag_attributes_{};

    std::function<void(Tokenizer &, Token &&)> on_emit_{};
    std::function<void(Tokenizer &, ParseError)> on_error_{};

    void emit(ParseError);
    void emit(Token &&);
    std::optional<char> consume_next_input_character();
    std::optional<char> peek_next_input_character() const;
    bool is_eof() const;

    std::vector<Attribute> &attributes_for_current_element();
    void start_attribute_in_current_tag_token(Attribute);
    Attribute &current_attribute();
    void reconsume_in(State);

    bool consumed_as_part_of_an_attribute() const;
    void flush_code_points_consumed_as_a_character_reference();
    void emit_temporary_buffer_as_character_tokens();
    bool is_appropriate_end_tag_token(Token const &) const;
    void emit_replacement_character();
};

} // namespace html2

#endif
