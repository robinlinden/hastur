// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef HTML2_TOKENIZER_H_
#define HTML2_TOKENIZER_H_

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

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

struct DoctypeToken {
    std::optional<std::string> name{std::nullopt};
    std::optional<std::string> public_identifier{std::nullopt};
    std::optional<std::string> system_identifier{std::nullopt};
    bool force_quirks{false};
    [[nodiscard]] bool operator==(DoctypeToken const &) const = default;
};

struct Attribute {
    std::string name{};
    std::string value{};
    [[nodiscard]] bool operator==(Attribute const &) const = default;
};

struct StartTagToken {
    std::string tag_name{};
    bool self_closing{false};
    std::vector<Attribute> attributes{};
    [[nodiscard]] bool operator==(StartTagToken const &) const = default;
};

struct EndTagToken {
    std::string tag_name{};
    bool self_closing{false};
    std::vector<Attribute> attributes{};
    [[nodiscard]] bool operator==(EndTagToken const &) const = default;
};

struct CommentToken {
    std::string data{};
    [[nodiscard]] bool operator==(CommentToken const &) const = default;
};

struct CharacterToken {
    char data{};
    [[nodiscard]] bool operator==(CharacterToken const &) const = default;
};

struct EndOfFileToken {
    [[nodiscard]] bool operator==(EndOfFileToken const &) const = default;
};

using Token = std::variant<DoctypeToken, StartTagToken, EndTagToken, CommentToken, CharacterToken, EndOfFileToken>;

std::string to_string(Token const &);

class Tokenizer {
public:
    Tokenizer(std::string_view input, std::function<void(Token &&, Tokenizer &)> on_emit)
        : input_{input}, on_emit_{std::move(on_emit)} {}

    void set_state(State);
    void run();

private:
    std::string_view input_;
    std::size_t pos_{0};
    State state_{State::Data};
    State return_state_{};
    Token current_token_{};

    std::string temporary_buffer_{};
    std::string last_start_tag_name_{};

    std::function<void(Token &&, Tokenizer &)> on_emit_{};

    void emit(Token &&);
    std::optional<char> consume_next_input_character();
    std::optional<char> peek_next_input_character() const;
    bool is_eof() const;

    void start_attribute_in_current_tag_token(Attribute);
    Attribute &current_attribute();
    void reconsume_in(State);

    bool consumed_as_part_of_an_attribute() const;
    void flush_code_points_consumed_as_a_character_reference();
    void emit_temporary_buffer_as_character_tokens();
    bool is_appropriate_end_tag_token(Token const &) const;
};

} // namespace html2

#endif
