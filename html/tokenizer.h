// SPDX-FileCopyrightText: 2021-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef HTML_TOKENIZER_H_
#define HTML_TOKENIZER_H_

#include "html/parse_error.h"
#include "html/token.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace html {

// https://html.spec.whatwg.org/multipage/parsing.html#tokenization
enum class State : std::uint8_t {
    Data,
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
    Token current_token_;

    std::string temporary_buffer_;
    std::string last_start_tag_name_;

    std::uint32_t character_reference_code_{};
    bool adjusted_current_node_in_html_namespace_{true};

    // These end-tag bits aren't allowed to leave the tokenizer, but we need to
    // keep them around internally to emit warnings when reasonable.
    bool self_closing_end_tag_detected_{false};
    std::vector<Attribute> end_tag_attributes_;

    std::function<void(Tokenizer &, Token &&)> on_emit_;
    std::function<void(Tokenizer &, ParseError)> on_error_;

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

} // namespace html

#endif
