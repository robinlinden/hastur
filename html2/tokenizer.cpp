// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "html2/tokenizer.h"

#include "html2/character_reference.h"
#include "util/string.h"
#include "util/unicode.h"

#include <spdlog/spdlog.h>

#include <cstdint>
#include <cstring>
#include <exception>
#include <limits>
#include <map>
#include <sstream>

using namespace std::literals;

namespace html2 {
namespace {

constexpr bool is_control(int code_point) {
    return code_point >= 0x7F && code_point <= 0x9F;
}

constexpr bool is_ascii_whitespace(int code_point) {
    switch (code_point) {
        case 0x09:
        case 0x0A:
        case 0x0C:
        case 0x0D:
        case 0x20:
            return true;
        default:
            return false;
    }
}

constexpr bool is_ascii_upper_alpha(char c) {
    return c >= 'A' && c <= 'Z';
}

constexpr bool is_ascii_lower_alpha(char c) {
    return c >= 'a' && c <= 'z';
}

constexpr bool is_ascii_alpha(char c) {
    return is_ascii_upper_alpha(c) || is_ascii_lower_alpha(c);
}

constexpr bool is_ascii_digit(char c) {
    return c >= '0' && c <= '9';
}

constexpr bool is_ascii_alphanumeric(char c) {
    return is_ascii_digit(c) || is_ascii_alpha(c);
}

constexpr char to_lower(char c) {
    return c + 0x20;
}

// https://infra.spec.whatwg.org/#surrogate
constexpr bool is_unicode_surrogate(int code_point) {
    return code_point >= 0xD800 && code_point <= 0xDFFF;
}

// https://infra.spec.whatwg.org/#noncharacter
constexpr bool is_unicode_noncharacter(int code_point) {
    if (code_point >= 0xFDD0 && code_point <= 0xFDEF) {
        return true;
    }

    switch (code_point) {
        case 0xFFFE:
        case 0xFFFF:
        case 0x1FFF:
        case 0x1FFFF:
        case 0x2FFFE:
        case 0x2FFFF:
        case 0x3FFFE:
        case 0x3FFFF:
        case 0x4FFFE:
        case 0x4FFFF:
        case 0x5FFFE:
        case 0x5FFFF:
        case 0x6FFFE:
        case 0x6FFFF:
        case 0x7FFFE:
        case 0x7FFFF:
        case 0x8FFFE:
        case 0x8FFFF:
        case 0x9FFFE:
        case 0x9FFFF:
        case 0xAFFFE:
        case 0xAFFFF:
        case 0xBFFFE:
        case 0xBFFFF:
        case 0xCFFFE:
        case 0xCFFFF:
        case 0xDFFFE:
        case 0xDFFFF:
        case 0xEFFFE:
        case 0xEFFFF:
        case 0xFFFFE:
        case 0xFFFFF:
        case 0x10FFFE:
        case 0x10FFFF:
            return true;
        default:
            return false;
    }
}

} // namespace

void Tokenizer::set_state(State state) {
    state_ = state;
}

void Tokenizer::run() {
    while (true) {
        if (input_.size() > pos_) {
            spdlog::trace("Running state {} w/ next char {}", static_cast<int>(state_), input_[pos_]);
        } else {
            spdlog::trace("Running state {} after input end", static_cast<int>(state_));
        }

        switch (state_) {
            case State::Data: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '&':
                        return_state_ = State::Data;
                        state_ = State::CharacterReference;
                        continue;
                    case '<':
                        state_ = State::TagOpen;
                        continue;
                    case '\0':
                        // This is an unexpected-null-character parse error.
                        emit(CharacterToken{*c});
                        continue;
                    default:
                        emit(CharacterToken{*c});
                        continue;
                }
                break;
            }

            case State::ScriptData: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '<':
                        state_ = State::ScriptDataLessThanSign;
                        continue;
                    case '\0':
                        // This is an unexpected-null-character parse error.
                        // TODO(mkiael): Emit replacement character for null
                        // emit(CharacterToken("\xFF\xFD"));
                        continue;
                    default:
                        emit(CharacterToken{*c});
                        continue;
                }
            }

            case State::TagOpen: {
                auto c = consume_next_input_character();
                if (!c) {
                    // This is an eof-before-tag-name parse error.
                    emit(CharacterToken{'<'});
                    emit(EndOfFileToken{});
                    return;
                }

                if (is_ascii_alpha(*c)) {
                    current_token_ = StartTagToken{};
                    reconsume_in(State::TagName);
                    continue;
                }

                switch (*c) {
                    case '!':
                        state_ = State::MarkupDeclarationOpen;
                        continue;
                    case '/':
                        state_ = State::EndTagOpen;
                        continue;
                    default:
                        std::terminate();
                }
                break;
            }

            case State::EndTagOpen: {
                auto c = consume_next_input_character();
                if (!c) {
                    // This is an eof-before-tag-name parse error.
                    emit(CharacterToken{'<'});
                    emit(CharacterToken{'/'});
                    emit(EndOfFileToken{});
                    return;
                }

                if (is_ascii_alpha(*c)) {
                    current_token_ = EndTagToken{};
                    reconsume_in(State::TagName);
                    continue;
                }

                std::terminate();
            }

            case State::TagName: {
                auto c = consume_next_input_character();
                if (!c) {
                    // This is an eof-in-tag parse error.
                    emit(EndOfFileToken{});
                    return;
                }

                auto append_to_tag_name = [&](auto text) {
                    if (std::holds_alternative<StartTagToken>(current_token_)) {
                        std::get<StartTagToken>(current_token_).tag_name += text;
                    } else {
                        std::get<EndTagToken>(current_token_).tag_name += text;
                    }
                };

                if (is_ascii_upper_alpha(*c)) {
                    append_to_tag_name(to_lower(*c));
                    continue;
                }

                switch (*c) {
                    case '\t':
                    case '\n':
                    case '\f':
                    case ' ':
                        state_ = State::BeforeAttributeName;
                        continue;
                    case '/':
                        state_ = State::SelfClosingStartTag;
                        continue;
                    case '>':
                        state_ = State::Data;
                        emit(std::move(current_token_));
                        continue;
                    case '\0':
                        // This is an unexpected-null-character parse error.
                        append_to_tag_name("\xFF\xFD");
                        continue;
                    default:
                        append_to_tag_name(*c);
                        continue;
                }
            }

            case State::ScriptDataLessThanSign: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(CharacterToken{'<'});
                    reconsume_in(State::ScriptData);
                    continue;
                }

                switch (*c) {
                    case '/':
                        temporary_buffer_ = "";
                        state_ = State::ScriptDataEndTagOpen;
                        continue;
                    case '!':
                        state_ = State::ScriptDataEscapeStart;
                        emit(CharacterToken{'<'});
                        emit(CharacterToken{'!'});
                        continue;
                    default:
                        emit(CharacterToken{'<'});
                        reconsume_in(State::ScriptData);
                        continue;
                }
            }

            case State::ScriptDataEndTagOpen: {
                auto c = consume_next_input_character();
                if (c && is_ascii_alpha(*c)) {
                    current_token_ = EndTagToken{};
                    reconsume_in(State::ScriptDataEndTagName);
                    continue;
                }

                emit(CharacterToken{'<'});
                emit(CharacterToken{'/'});
                reconsume_in(State::ScriptData);
                continue;
            }

            case State::ScriptDataEndTagName: {
                auto c = consume_next_input_character();

                auto anything_else = [this] {
                    emit(CharacterToken{'<'});
                    emit(CharacterToken{'/'});
                    emit_temporary_buffer_as_character_tokens();
                    reconsume_in(State::ScriptData);
                };

                if (!c) {
                    anything_else();
                    continue;
                }

                if (is_ascii_upper_alpha(*c)) {
                    std::get<EndTagToken>(current_token_).tag_name.append(1, to_lower(*c));
                    temporary_buffer_.append(1, *c);
                    continue;
                }

                if (is_ascii_lower_alpha(*c)) {
                    std::get<EndTagToken>(current_token_).tag_name.append(1, *c);
                    temporary_buffer_.append(1, *c);
                    continue;
                }

                switch (*c) {
                    case '\t':
                    case '\n':
                    case '\f':
                    case ' ':
                        if (is_appropriate_end_tag_token(current_token_)) {
                            state_ = State::BeforeAttributeName;
                        } else {
                            anything_else();
                        }
                        continue;
                    case '/':
                        if (is_appropriate_end_tag_token(current_token_)) {
                            state_ = State::SelfClosingStartTag;
                        } else {
                            anything_else();
                        }
                        continue;
                    case '>':
                        if (is_appropriate_end_tag_token(current_token_)) {
                            state_ = State::Data;
                            emit(std::move(current_token_));
                        } else {
                            anything_else();
                        }
                        continue;
                    default:
                        anything_else();
                        continue;
                }
            }

            case State::ScriptDataEscapeStart: {
                auto c = consume_next_input_character();
                if (!c) {
                    reconsume_in(State::ScriptData);
                    continue;
                }

                switch (*c) {
                    case '-':
                        state_ = State::ScriptDataEscapeStartDash;
                        emit(CharacterToken{*c});
                        continue;
                    default:
                        reconsume_in(State::ScriptData);
                        continue;
                }
            }

            case State::ScriptDataEscapeStartDash: {
                auto c = consume_next_input_character();
                if (!c) {
                    reconsume_in(State::ScriptData);
                    continue;
                }

                switch (*c) {
                    case '-':
                        state_ = State::ScriptDataEscapedDashDash;
                        emit(CharacterToken{*c});
                        continue;
                    default:
                        reconsume_in(State::ScriptData);
                        continue;
                }
            }

            case State::ScriptDataEscaped: {
                auto c = consume_next_input_character();
                if (!c) {
                    // This is an eof-in-script-html-comment-like-text parse error.
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '-':
                        state_ = State::ScriptDataEscapedDash;
                        emit(CharacterToken{*c});
                        continue;
                    case '<':
                        state_ = State::ScriptDataEscapedLessThanSign;
                        continue;
                    case '\0':
                        // This is an unexpected-null-character parse error.
                        // TODO(mkiael): Emit replacement character for null
                        // emit(CharacterToken("\xFF\xFD"));
                        continue;
                    default:
                        emit(CharacterToken{*c});
                        continue;
                }
            }

            case State::ScriptDataEscapedDash: {
                auto c = consume_next_input_character();
                if (!c) {
                    // This is an eof-in-script-html-comment-like-text parse error.
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '-':
                        state_ = State::ScriptDataEscapedDashDash;
                        emit(CharacterToken{*c});
                        continue;
                    case '<':
                        state_ = State::ScriptDataEscapedLessThanSign;
                        continue;
                    case '\0':
                        // This is an unexpected-null-character parse error.
                        state_ = State::ScriptDataEscaped;
                        // TODO(mkiael): Emit replacement character for null
                        // emit(CharacterToken("\xFF\xFD"));
                        continue;
                    default:
                        state_ = State::ScriptDataEscaped;
                        emit(CharacterToken{*c});
                        continue;
                }
            }

            case State::ScriptDataEscapedDashDash: {
                auto c = consume_next_input_character();
                if (!c) {
                    // This is an eof-in-script-html-comment-like-text parse error.
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '-':
                        emit(CharacterToken{*c});
                        continue;
                    case '<':
                        state_ = State::ScriptDataEscapedLessThanSign;
                        continue;
                    case '>':
                        state_ = State::ScriptData;
                        emit(CharacterToken{*c});
                        continue;
                    case '\0':
                        // This is an unexpected-null-character parse error.
                        state_ = State::ScriptDataEscaped;
                        // TODO(mkiael): Emit replacement character for null
                        // emit(CharacterToken("\xFF\xFD"));
                        continue;
                    default:
                        state_ = State::ScriptDataEscaped;
                        emit(CharacterToken{*c});
                        continue;
                }
            }

            case State::ScriptDataEscapedLessThanSign: {
                auto c = consume_next_input_character();
                if (c == '/') {
                    temporary_buffer_ = "";
                    state_ = State::ScriptDataEscapedEndTagOpen;
                    continue;
                }

                if (c && is_ascii_alpha(*c)) {
                    temporary_buffer_ = "";
                    emit(CharacterToken{'<'});
                    reconsume_in(State::ScriptDataDoubleEscapeStart);
                    continue;
                }

                emit(CharacterToken{'<'});
                reconsume_in(State::ScriptDataEscaped);
                continue;
            }

            case State::ScriptDataEscapedEndTagOpen: {
                auto c = consume_next_input_character();
                if (c && is_ascii_alpha(*c)) {
                    current_token_ = EndTagToken{};
                    reconsume_in(State::ScriptDataEscapedEndTagName);
                    continue;
                }

                emit(CharacterToken{'<'});
                emit(CharacterToken{'/'});
                reconsume_in(State::ScriptDataEscaped);
                continue;
            }

            case State::ScriptDataEscapedEndTagName: {
                auto c = consume_next_input_character();

                auto anything_else = [this] {
                    emit(CharacterToken{'<'});
                    emit(CharacterToken{'/'});
                    emit_temporary_buffer_as_character_tokens();
                    reconsume_in(State::ScriptDataEscaped);
                };

                if (!c) {
                    anything_else();
                    continue;
                }

                if (is_ascii_upper_alpha(*c)) {
                    std::get<EndTagToken>(current_token_).tag_name.append(1, to_lower(*c));
                    temporary_buffer_.append(1, *c);
                    continue;
                }

                if (is_ascii_lower_alpha(*c)) {
                    std::get<EndTagToken>(current_token_).tag_name.append(1, *c);
                    temporary_buffer_.append(1, *c);
                    continue;
                }

                switch (*c) {
                    case '\t':
                    case '\n':
                    case '\f':
                    case ' ':
                        if (is_appropriate_end_tag_token(current_token_)) {
                            state_ = State::BeforeAttributeName;
                        } else {
                            anything_else();
                        }
                        continue;
                    case '/':
                        if (is_appropriate_end_tag_token(current_token_)) {
                            state_ = State::SelfClosingStartTag;
                        } else {
                            anything_else();
                        }
                        continue;
                    case '>':
                        if (is_appropriate_end_tag_token(current_token_)) {
                            state_ = State::Data;
                            emit(std::move(current_token_));
                        } else {
                            anything_else();
                        }
                        continue;
                    default:
                        anything_else();
                        continue;
                }
            }

            case State::ScriptDataDoubleEscapeStart: {
                auto c = consume_next_input_character();
                if (!c) {
                    reconsume_in(State::ScriptDataEscaped);
                    continue;
                }

                if (is_ascii_upper_alpha(*c)) {
                    temporary_buffer_.append(1, to_lower(*c));
                    emit(CharacterToken{*c});
                    continue;
                }

                if (is_ascii_lower_alpha(*c)) {
                    temporary_buffer_.append(1, *c);
                    emit(CharacterToken{*c});
                    continue;
                }

                switch (*c) {
                    case '\t':
                    case '\n':
                    case '\f':
                    case ' ':
                    case '/':
                    case '>':
                        if (temporary_buffer_ == "script"sv) {
                            state_ = State::ScriptDataDoubleEscaped;
                        } else {
                            state_ = State::ScriptDataEscaped;
                        }
                        emit(CharacterToken{*c});
                        continue;
                    default:
                        reconsume_in(State::ScriptDataEscaped);
                        continue;
                }
            }

            case State::ScriptDataDoubleEscaped: {
                auto c = consume_next_input_character();
                if (!c) {
                    // This is an eof-in-script-html-comment-like-text parse error.
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '-':
                        state_ = State::ScriptDataDoubleEscapedDash;
                        emit(CharacterToken{*c});
                        continue;
                    case '<':
                        state_ = State::ScriptDataDoubleEscapedLessThanSign;
                        emit(CharacterToken{*c});
                        continue;
                    case '\0':
                        // This is an unexpected-null-character parse error.
                        // TODO(mkiael): Emit replacement character for null
                        // emit(CharacterToken("\xFF\xFD"));
                        continue;
                    default:
                        emit(CharacterToken{*c});
                        continue;
                }
            }

            case State::ScriptDataDoubleEscapedDash: {
                auto c = consume_next_input_character();
                if (!c) {
                    // This is an eof-in-script-html-comment-like-text parse error.
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '-':
                        state_ = State::ScriptDataDoubleEscapedDashDash;
                        emit(CharacterToken{*c});
                        continue;
                    case '<':
                        state_ = State::ScriptDataDoubleEscapedLessThanSign;
                        emit(CharacterToken{*c});
                        continue;
                    case '\0':
                        // This is an unexpected-null-character parse error.
                        state_ = State::ScriptDataDoubleEscaped;
                        // TODO(mkiael): Emit replacement character for null
                        // emit(CharacterToken("\xFF\xFD"));
                        continue;
                    default:
                        state_ = State::ScriptDataDoubleEscaped;
                        emit(CharacterToken{*c});
                        continue;
                }
            }

            case State::ScriptDataDoubleEscapedDashDash: {
                auto c = consume_next_input_character();
                if (!c) {
                    // This is an eof-in-script-html-comment-like-text parse error.
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '-':
                        emit(CharacterToken{*c});
                        continue;
                    case '<':
                        state_ = State::ScriptDataDoubleEscapedLessThanSign;
                        emit(CharacterToken{*c});
                        continue;
                    case '>':
                        state_ = State::ScriptData;
                        emit(CharacterToken{*c});
                        continue;
                    case '\0':
                        // This is an unexpected-null-character parse error.
                        state_ = State::ScriptDataDoubleEscaped;
                        // TODO(mkiael): Emit replacement character for null
                        // emit(CharacterToken("\xFF\xFD"));
                        continue;
                    default:
                        state_ = State::ScriptDataDoubleEscaped;
                        emit(CharacterToken{*c});
                        continue;
                }
            }

            case State::ScriptDataDoubleEscapedLessThanSign: {
                auto c = consume_next_input_character();
                if (c == '/') {
                    temporary_buffer_ = "";
                    state_ = State::ScriptDataDoubleEscapeEnd;
                    emit(CharacterToken{*c});
                    continue;
                }

                reconsume_in(State::ScriptDataDoubleEscaped);
                continue;
            }

            case State::ScriptDataDoubleEscapeEnd: {
                auto c = consume_next_input_character();
                if (!c) {
                    reconsume_in(State::ScriptDataDoubleEscaped);
                    continue;
                }

                if (is_ascii_upper_alpha(*c)) {
                    temporary_buffer_.append(1, to_lower(*c));
                    emit(CharacterToken{*c});
                    continue;
                }

                if (is_ascii_lower_alpha(*c)) {
                    temporary_buffer_.append(1, *c);
                    emit(CharacterToken{*c});
                    continue;
                }

                switch (*c) {
                    case '\t':
                    case '\n':
                    case '\f':
                    case ' ':
                    case '/':
                    case '>':
                        if (temporary_buffer_ == "script"sv) {
                            state_ = State::ScriptDataEscaped;
                        } else {
                            state_ = State::ScriptDataDoubleEscaped;
                        }
                        emit(CharacterToken{*c});
                        continue;
                    default:
                        reconsume_in(State::ScriptDataDoubleEscaped);
                        continue;
                }
            }

            case State::BeforeAttributeName: {
                auto c = consume_next_input_character();
                if (!c || *c == '/' || *c == '>') {
                    reconsume_in(State::AfterAttributeName);
                    continue;
                }

                switch (*c) {
                    case '\t':
                    case '\n':
                    case '\f':
                    case ' ':
                        continue;
                    case '=':
                        // This is an unexpected-equals-sign-before-attribute-name parse error.
                        start_attribute_in_current_tag_token({.name = "="});
                        state_ = State::AttributeName;
                        continue;
                    default:
                        start_attribute_in_current_tag_token({});
                        reconsume_in(State::AttributeName);
                        continue;
                }
            }

            case State::AttributeName: {
                auto c = consume_next_input_character();
                if (!c || *c == '\t' || *c == '\n' || *c == '\f' || *c == ' ' || *c == '/' || *c == '>') {
                    reconsume_in(State::AfterAttributeName);
                    continue;
                }

                auto append_to_current_attribute_name = [&](auto text) {
                    if (std::holds_alternative<StartTagToken>(current_token_)) {
                        std::get<StartTagToken>(current_token_).attributes.back().name += text;
                    } else {
                        std::get<EndTagToken>(current_token_).attributes.back().name += text;
                    }
                };

                if (is_ascii_upper_alpha(*c)) {
                    append_to_current_attribute_name(to_lower(*c));
                    continue;
                }

                switch (*c) {
                    case '=':
                        state_ = State::BeforeAttributeValue;
                        continue;
                    case '\0':
                        // This is an unexpected-null-character parse error.
                        append_to_current_attribute_name("\xFF\xFD");
                        continue;
                    case '"':
                    case '\'':
                    case '<':
                        // This is an unexpected-character-in-attribute-name parse error.
                    default:
                        append_to_current_attribute_name(*c);
                        continue;
                }
            }

            case State::AfterAttributeName: {
                auto c = consume_next_input_character();
                if (!c) {
                    // This is an eof-in-tag parse error.
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '\t':
                    case '\n':
                    case '\f':
                    case ' ':
                        continue;
                    case '/':
                        state_ = State::SelfClosingStartTag;
                        continue;
                    case '=':
                        state_ = State::BeforeAttributeValue;
                        continue;
                    case '>':
                        state_ = State::Data;
                        emit(std::move(current_token_));
                        continue;
                    default:
                        start_attribute_in_current_tag_token({});
                        reconsume_in(State::AttributeName);
                        continue;
                }
            }

            case State::BeforeAttributeValue: {
                auto c = consume_next_input_character();
                if (!c) {
                    reconsume_in(State::AttributeValueUnquoted);
                    continue;
                }

                switch (*c) {
                    case '\t':
                    case '\n':
                    case '\f':
                    case ' ':
                        continue;
                    case '"':
                        state_ = State::AttributeValueDoubleQuoted;
                        continue;
                    case '\'':
                        state_ = State::AttributeValueSingleQuoted;
                        continue;
                    case '>':
                        // This is a missing-attribute-value parse error.
                        state_ = State::Data;
                        emit(std::move(current_token_));
                        continue;
                    default:
                        reconsume_in(State::AttributeValueUnquoted);
                        continue;
                }
            }

            case State::AttributeValueDoubleQuoted: {
                auto c = consume_next_input_character();
                if (!c) {
                    // This is an eof-in-tag parse error.
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '"':
                        state_ = State::AfterAttributeValueQuoted;
                        continue;
                    case '&':
                        return_state_ = State::AttributeValueDoubleQuoted;
                        state_ = State::CharacterReference;
                        continue;
                    case '\0':
                        // This is an unexpected-null-character parse error.
                        current_attribute().value += "\xFF\xFD";
                        continue;
                    default:
                        current_attribute().value += *c;
                        continue;
                }
            }

            case State::AttributeValueSingleQuoted: {
                auto c = consume_next_input_character();
                if (!c) {
                    // This is an eof-in-tag parse error.
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '\'':
                        state_ = State::AfterAttributeValueQuoted;
                        continue;
                    case '&':
                        return_state_ = State::AttributeValueSingleQuoted;
                        state_ = State::CharacterReference;
                        continue;
                    case '\0':
                        // This is an unexpected-null-character parse error.
                        current_attribute().value += "\xFF\xFD";
                        continue;
                    default:
                        current_attribute().value += *c;
                        continue;
                }
            }

            case State::AttributeValueUnquoted: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::EofInTag);
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '\t':
                    case '\n':
                    case '\f':
                    case ' ':
                        state_ = State::BeforeAttributeName;
                        continue;
                    case '&':
                        return_state_ = State::AttributeValueUnquoted;
                        state_ = State::CharacterReference;
                        continue;
                    case '>':
                        state_ = State::Data;
                        emit(std::move(current_token_));
                        continue;
                    case '\0':
                        emit(ParseError::UnexpectedNullCharacter);
                        current_attribute().value.append(util::unicode_to_utf8(0xFFFD));
                        continue;
                    case '"':
                    case '\'':
                    case '<':
                    case '=':
                    case '`':
                        emit(ParseError::UnexpectedCharacterInUnquotedAttributeValue);
                        [[fallthrough]];
                    default:
                        current_attribute().value += *c;
                        continue;
                }
            }

            case State::AfterAttributeValueQuoted: {
                auto c = consume_next_input_character();
                if (!c) {
                    // This is an eof-in-tag parse error.
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '\t':
                    case '\n':
                    case '\f':
                    case ' ':
                        state_ = State::BeforeAttributeName;
                        continue;
                    case '/':
                        state_ = State::SelfClosingStartTag;
                        continue;
                    case '>':
                        state_ = State::Data;
                        emit(std::move(current_token_));
                        continue;
                    default:
                        // This is a missing-whitespace-between-attributes parse error.
                        reconsume_in(State::BeforeAttributeName);
                        continue;
                }
            }

            case State::SelfClosingStartTag: {
                auto c = consume_next_input_character();
                if (!c) {
                    // This is an eof-in-tag parse error.
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '>':
                        if (std::holds_alternative<StartTagToken>(current_token_)) {
                            std::get<StartTagToken>(current_token_).self_closing = true;
                        } else {
                            std::get<EndTagToken>(current_token_).self_closing = true;
                        }
                        state_ = State::Data;
                        emit(std::move(current_token_));
                        continue;
                    default:
                        // This is a missing-whitespace-between-attributes parse error.
                        reconsume_in(State::BeforeAttributeName);
                        continue;
                }
            }

            case State::MarkupDeclarationOpen:
                if (input_.substr(pos_, 2) == "--") {
                    pos_ += 2;
                    current_token_ = CommentToken{.data = std::string{}};
                    state_ = State::CommentStart;
                    continue;
                }

                if (util::no_case_compare(input_.substr(pos_, std::strlen("DOCTYPE")), "doctype"sv)) {
                    pos_ += std::strlen("DOCTYPE");
                    state_ = State::Doctype;
                    continue;
                }
                break;

            case State::CommentStart: {
                auto c = consume_next_input_character();
                if (!c) {
                    reconsume_in(State::Comment);
                    continue;
                }

                switch (*c) {
                    case '-':
                        state_ = State::CommentStartDash;
                        continue;
                    case '>':
                        // This is an abrupt closing of empty comment parse error.
                        state_ = State::Data;
                        emit(std::move(current_token_));
                        continue;
                    default:
                        reconsume_in(State::Comment);
                        continue;
                }
            }

            case State::CommentStartDash: {
                auto c = consume_next_input_character();
                if (!c) {
                    // This is an eof-in-comment parse error.
                    emit(std::move(current_token_));
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '-':
                        state_ = State::CommentEnd;
                        continue;
                    case '>':
                        // This is an abrupt closing of empty comment parse error.
                        state_ = State::Data;
                        emit(std::move(current_token_));
                        continue;
                    default:
                        std::get<CommentToken>(current_token_).data.append("-");
                        reconsume_in(State::Comment);
                        continue;
                }
            }

            case State::Comment: {
                auto c = consume_next_input_character();
                if (!c) {
                    // This is an eof-in-comment parse error.
                    emit(std::move(current_token_));
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '<':
                        std::get<CommentToken>(current_token_).data.append(1, *c);
                        state_ = State::CommentLessThanSign;
                        continue;
                    case '-':
                        state_ = State::CommentEndDash;
                        continue;
                    case '\0':
                        // This is an unexpected-null-character parse error.
                        std::get<CommentToken>(current_token_).data.append("\xFF\xFD");
                        continue;
                    default:
                        std::get<CommentToken>(current_token_).data.append(1, *c);
                        continue;
                }
            }

            case State::CommentLessThanSign: {
                auto c = consume_next_input_character();
                if (!c) {
                    reconsume_in(State::Comment);
                    continue;
                }

                switch (*c) {
                    case '!':
                        std::get<CommentToken>(current_token_).data.append(1, *c);
                        state_ = State::CommentLessThanSignBang;
                        continue;
                    case '<':
                        std::get<CommentToken>(current_token_).data.append(1, *c);
                        continue;
                    default:
                        reconsume_in(State::Comment);
                        continue;
                }
            }

            case State::CommentLessThanSignBang: {
                auto c = consume_next_input_character();
                if (!c) {
                    reconsume_in(State::Comment);
                    continue;
                }

                switch (*c) {
                    case '-':
                        state_ = State::CommentLessThanSignBangDash;
                        continue;
                    default:
                        reconsume_in(State::Comment);
                        continue;
                }
            }

            case State::CommentLessThanSignBangDash: {
                auto c = consume_next_input_character();
                if (!c) {
                    reconsume_in(State::CommentEndDash);
                    continue;
                }

                switch (*c) {
                    case '-':
                        state_ = State::CommentLessThanSignBangDashDash;
                        continue;
                    default:
                        reconsume_in(State::CommentEndDash);
                        continue;
                }
            }

            case State::CommentLessThanSignBangDashDash: {
                auto c = consume_next_input_character();
                if (!c) {
                    reconsume_in(State::CommentEnd);
                    continue;
                }

                switch (*c) {
                    case '>':
                        reconsume_in(State::CommentEnd);
                        continue;
                    default:
                        // This is a nested-comment parse error.
                        reconsume_in(State::CommentEnd);
                        continue;
                }
            }

            case State::CommentEndDash: {
                auto c = consume_next_input_character();
                if (!c) {
                    // This is an eof-in-comment parse error.
                    emit(std::move(current_token_));
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '-':
                        state_ = State::CommentEnd;
                        continue;
                    default:
                        std::get<CommentToken>(current_token_).data.append("-");
                        reconsume_in(State::Comment);
                        continue;
                }
            }

            case State::CommentEnd: {
                auto c = consume_next_input_character();
                if (!c) {
                    // This is an eof-in-comment parse error.
                    emit(std::move(current_token_));
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '>':
                        state_ = State::Data;
                        emit(std::move(current_token_));
                        continue;
                    case '!':
                        state_ = State::CommentEndBang;
                        continue;
                    case '-':
                        std::get<CommentToken>(current_token_).data.append("-");
                        continue;
                    default:
                        std::get<CommentToken>(current_token_).data.append("--");
                        reconsume_in(State::Comment);
                        continue;
                }
            }

            case State::CommentEndBang: {
                auto c = consume_next_input_character();
                if (!c) {
                    // This is an eof-in-comment parse error.
                    emit(std::move(current_token_));
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '-':
                        std::get<CommentToken>(current_token_).data.append("--!");
                        state_ = State::CommentEndDash;
                        continue;
                    case '>':
                        // This is an incorrectly-closed-comment parse error.
                        state_ = State::Data;
                        emit(std::move(current_token_));
                        continue;
                    default:
                        std::get<CommentToken>(current_token_).data.append("--!");
                        reconsume_in(State::Comment);
                        continue;
                }
            }

            case State::Doctype: {
                auto c = consume_next_input_character();
                if (!c) {
                    // This is an eof-in-doctype parse error.
                    emit(DoctypeToken{.force_quirks = true});
                    emit(EndOfFileToken{});
                    return;
                }

                switch (*c) {
                    case '\t':
                    case '\n':
                    case '\f':
                    case ' ':
                        state_ = State::BeforeDoctypeName;
                        continue;
                    default:
                        std::terminate();
                }
            }

            case State::BeforeDoctypeName: {
                auto c = consume_next_input_character();
                if (!c) {
                    // This is an eof-in-doctype parse error.
                    emit(DoctypeToken{.force_quirks = true});
                    emit(EndOfFileToken{});
                    return;
                }

                if (is_ascii_upper_alpha(*c)) {
                    current_token_ = DoctypeToken{.name = std::string{}};
                    std::get<DoctypeToken>(current_token_).name->append(1, to_lower(*c));
                    state_ = State::DoctypeName;
                    continue;
                }

                switch (*c) {
                    case '\t':
                    case '\n':
                    case '\f':
                    case ' ':
                        continue;
                    case '\0':
                        // This is an unexpected-null-character parse error.
                        current_token_ = DoctypeToken{.name = std::string{}};
                        std::get<DoctypeToken>(current_token_).name->append("\xFF\xFD");
                        state_ = State::DoctypeName;
                        continue;
                    case '>':
                        // This is a missing-doctype-name parse error.
                        current_token_ = DoctypeToken{.force_quirks = true};
                        state_ = State::Data;
                        emit(std::move(current_token_));
                        continue;
                    default:
                        current_token_ = DoctypeToken{.name = std::string{}};
                        std::get<DoctypeToken>(current_token_).name->append(1, *c);
                        state_ = State::DoctypeName;
                        continue;
                }
            }

            case State::DoctypeName: {
                auto c = consume_next_input_character();
                if (!c) {
                    // This is an eof-in-doctype parse error.
                    std::get<DoctypeToken>(current_token_).force_quirks = true;
                    emit(std::move(current_token_));
                    emit(EndOfFileToken{});
                    return;
                }

                if (is_ascii_upper_alpha(*c)) {
                    std::get<DoctypeToken>(current_token_).name->append(1, to_lower(*c));
                    continue;
                }

                switch (*c) {
                    case '\t':
                    case '\n':
                    case '\f':
                    case ' ':
                        state_ = State::AfterDoctypeName;
                        continue;
                    case '>':
                        state_ = State::Data;
                        emit(std::move(current_token_));
                        continue;
                    case '\0':
                        // This is an unexpected-null-character parse error.
                        std::get<DoctypeToken>(current_token_).name->append("\xFF\xFD");
                        continue;
                    default:
                        std::get<DoctypeToken>(current_token_).name->append(1, *c);
                        continue;
                }
            }

            case State::CharacterReference: {
                temporary_buffer_ = "&"s;

                auto c = consume_next_input_character();
                if (!c) {
                    flush_code_points_consumed_as_a_character_reference();
                    reconsume_in(return_state_);
                    continue;
                }

                if (is_ascii_alphanumeric(*c)) {
                    reconsume_in(State::NamedCharacterReference);
                    continue;
                }

                switch (*c) {
                    case '#':
                        temporary_buffer_.append(1, *c);
                        state_ = State::NumericCharacterReference;
                        continue;
                    default:
                        flush_code_points_consumed_as_a_character_reference();
                        reconsume_in(return_state_);
                        continue;
                }
            }

            case State::NamedCharacterReference: {
                // TODO(robinlinden): -1 here isn't great, but it works right now.
                auto maybe_reference = find_named_character_reference_for(input_.substr(pos_ - 1));
                if (!maybe_reference) {
                    flush_code_points_consumed_as_a_character_reference();
                    state_ = State::AmbiguousAmpersand;
                    continue;
                }

                // -1 because of the TODO above.
                pos_ += maybe_reference->name.size() - 1;
                // Should be appending, but again, the TODO.
                temporary_buffer_ = maybe_reference->name;

                auto c = peek_next_input_character();
                if (c.has_value() && consumed_as_part_of_an_attribute() && temporary_buffer_.back() != ';'
                        && (c == '=' || is_ascii_alphanumeric(*c))) {
                    flush_code_points_consumed_as_a_character_reference();
                    state_ = return_state_;
                    continue;
                }

                if (temporary_buffer_.back() != ';') {
                    // This is a missing-semicolon-after-character-reference parse error.
                }

                temporary_buffer_.clear();
                temporary_buffer_.append(util::unicode_to_utf8(maybe_reference->first_codepoint));
                if (maybe_reference->second_codepoint) {
                    temporary_buffer_.append(util::unicode_to_utf8(*maybe_reference->second_codepoint));
                }

                flush_code_points_consumed_as_a_character_reference();
                state_ = return_state_;
                continue;
            }

            case State::AmbiguousAmpersand: {
                auto c = consume_next_input_character();
                if (!c) {
                    reconsume_in(return_state_);
                    continue;
                }

                if (is_ascii_alphanumeric(*c)) {
                    if (consumed_as_part_of_an_attribute()) {
                        current_attribute().value += *c;
                    } else {
                        emit(CharacterToken{*c});
                    }
                    continue;
                }

                if (*c == ';') {
                    // This is an unknown-named-character-reference parse error.
                }

                reconsume_in(return_state_);
                continue;
            }

            case State::NumericCharacterReference: {
                character_reference_code_ = 0;
                auto c = consume_next_input_character();
                if (!c) {
                    reconsume_in(State::DecimalCharacterReferenceStart);
                    continue;
                }

                switch (*c) {
                    case 'x':
                    case 'X':
                        temporary_buffer_ += *c;
                        state_ = State::HexadecimalCharacterReferenceStart;
                        continue;
                    default:
                        reconsume_in(State::DecimalCharacterReferenceStart);
                        continue;
                }
            }

            case State::DecimalCharacterReferenceStart: {
                auto c = consume_next_input_character();
                if (!c || !is_ascii_digit(*c)) {
                    // This is an absence-of-digits-in-numeric-character-reference parse error.
                    flush_code_points_consumed_as_a_character_reference();
                    reconsume_in(return_state_);
                    continue;
                }

                reconsume_in(State::DecimalCharacterReference);
                continue;
            }

            case State::DecimalCharacterReference: {
                auto c = consume_next_input_character();
                if (!c) {
                    // This is a missing-semicolon-after-character-reference parse error.
                    reconsume_in(State::NumericCharacterReferenceEnd);
                    continue;
                }

                if (is_ascii_digit(*c)) {
                    character_reference_code_ *= 10;
                    character_reference_code_ += *c - 0x30;
                    continue;
                }

                if (*c == ';') {
                    state_ = State::NumericCharacterReferenceEnd;
                    continue;
                }

                // This is a missing-semicolon-after-character-reference parse error.
                reconsume_in(State::NumericCharacterReferenceEnd);
                continue;
            }

            case State::NumericCharacterReferenceEnd: {
                if (character_reference_code_ == 0) {
                    // This is a null-character-reference parse error.
                    character_reference_code_ = 0xFFFD;
                }

                if (character_reference_code_ > 0x10FFFF) {
                    // This is a character-reference-outside-unicode-range parse error.
                    character_reference_code_ = 0xFFFD;
                }

                if (is_unicode_surrogate(character_reference_code_)) {
                    // This is a surrogate-character-reference parse error.
                    character_reference_code_ = 0xFFFD;
                }

                if (is_unicode_noncharacter(character_reference_code_)) {
                    // This is a noncharacter-character-reference parse error.
                    character_reference_code_ = 0xFFFD;
                }

                if (character_reference_code_ == 0x0D
                        || (is_control(character_reference_code_) && !is_ascii_whitespace(character_reference_code_))) {
                    // This is a control-character-reference parse error.
                }

                static std::map<std::uint32_t, std::uint32_t> const replacements{{0x80, 0x20AC},
                        {0x82, 0x201A},
                        {0x83, 0x0192},
                        {0x84, 0x201E},
                        {0x85, 0x2026},
                        {0x86, 0x2020},
                        {0x87, 0x2021},
                        {0x88, 0x02C6},
                        {0x89, 0x2030},
                        {0x8A, 0x0160},
                        {0x8B, 0x2039},
                        {0x8C, 0x0152},
                        {0x8E, 0x017D},
                        {0x91, 0x2018},
                        {0x92, 0x2019},
                        {0x93, 0x201C},
                        {0x94, 0x201D},
                        {0x95, 0x2022},
                        {0x96, 0x2013},
                        {0x97, 0x2014},
                        {0x98, 0x02DC},
                        {0x99, 0x2122},
                        {0x9A, 0x0161},
                        {0x9B, 0x203A},
                        {0x9C, 0x0153},
                        {0x9E, 0x017E},
                        {0x9F, 0x0178}};

                if (replacements.contains(character_reference_code_)) {
                    character_reference_code_ = replacements.at(character_reference_code_);
                }

                temporary_buffer_ = util::unicode_to_utf8(character_reference_code_);
                flush_code_points_consumed_as_a_character_reference();
                state_ = return_state_;
                continue;
            }

            default:
                std::terminate();
        }
    }
}

void Tokenizer::emit(ParseError error) {
    on_error_(*this, error);
}

void Tokenizer::emit(Token &&token) {
    if (std::holds_alternative<StartTagToken>(token)) {
        last_start_tag_name_ = std::get<StartTagToken>(token).tag_name;
    }
    on_emit_(*this, std::move(token));
}

std::optional<char> Tokenizer::consume_next_input_character() {
    if (is_eof()) {
        pos_ += 1;
        return std::nullopt;
    }

    return input_[pos_++];
}

std::optional<char> Tokenizer::peek_next_input_character() const {
    if (is_eof()) {
        return std::nullopt;
    }

    return input_[pos_];
}

bool Tokenizer::is_eof() const {
    return pos_ >= input_.size();
}

void Tokenizer::start_attribute_in_current_tag_token(Attribute attr) {
    if (std::holds_alternative<StartTagToken>(current_token_)) {
        std::get<StartTagToken>(current_token_).attributes.push_back(std::move(attr));
    } else {
        std::get<EndTagToken>(current_token_).attributes.push_back(std::move(attr));
    }
}

Attribute &Tokenizer::current_attribute() {
    if (std::holds_alternative<StartTagToken>(current_token_)) {
        return std::get<StartTagToken>(current_token_).attributes.back();
    } else {
        return std::get<EndTagToken>(current_token_).attributes.back();
    }
}

void Tokenizer::reconsume_in(State state) {
    --pos_;
    state_ = state;
}

bool Tokenizer::consumed_as_part_of_an_attribute() const {
    return return_state_ == State::AttributeValueDoubleQuoted || return_state_ == State::AttributeValueSingleQuoted
            || return_state_ == State::AttributeValueUnquoted;
}

void Tokenizer::flush_code_points_consumed_as_a_character_reference() {
    if (consumed_as_part_of_an_attribute()) {
        current_attribute().value += temporary_buffer_;
        return;
    }

    emit_temporary_buffer_as_character_tokens();
}

void Tokenizer::emit_temporary_buffer_as_character_tokens() {
    for (char c : temporary_buffer_) {
        emit(CharacterToken{c});
    }
}

bool Tokenizer::is_appropriate_end_tag_token(Token const &token) const {
    if (std::holds_alternative<EndTagToken>(token)) {
        return std::get<EndTagToken>(token).tag_name == last_start_tag_name_;
    }
    return false;
}

} // namespace html2
