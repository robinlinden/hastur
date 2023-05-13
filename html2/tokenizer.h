// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef HTML2_TOKENIZER_H_
#define HTML2_TOKENIZER_H_

#include "html2/token.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace html2 {

// https://html.spec.whatwg.org/multipage/parsing.html#tokenization
enum class State {
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

enum class ParseError {
    AbruptClosingOfEmptyComment,
    AbruptDoctypePublicIdentifier,
    AbruptDoctypeSystemIdentifier,
    AbsenceOfDigitsInNumericCharacterReference,
    CdataInHtmlContent,
    CharacterReferenceOutsideUnicodeRange,
    ControlCharacterReference,
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
    UnknownNamedCharacterReference,
};

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
    void set_adjusted_current_node_not_in_html_namespace(bool in_html_namespace) {
        adjusted_current_node_not_in_html_namespace_ = in_html_namespace;
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
    bool adjusted_current_node_not_in_html_namespace_{false};

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
