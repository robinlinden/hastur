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
#include <sstream>

using namespace std::literals;

namespace html2 {
namespace {

constexpr bool is_ascii_upper_alpha(char c) {
    return c >= 'A' && c <= 'Z';
}

constexpr bool is_ascii_lower_alpha(char c) {
    return c >= 'a' && c <= 'z';
}

constexpr bool is_ascii_alpha(char c) {
    return is_ascii_upper_alpha(c) || is_ascii_lower_alpha(c);
}

constexpr bool is_numeric(char c) {
    return c >= '0' && c <= '9';
}

constexpr bool is_ascii_alphanumeric(char c) {
    return is_numeric(c) || is_ascii_alpha(c);
}

constexpr char to_lower(char c) {
    return c + 0x20;
}

template<class... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};

// Not needed as of C++20, but gcc 10 won't work without it.
template<class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

} // namespace

std::string to_string(Token const &token) {
    auto try_print = []<typename T>(std::ostream &os, std::optional<T> maybe_value) {
        if (maybe_value) {
            os << ' ' << *maybe_value;
        } else {
            os << " \"\"";
        }
    };

    std::stringstream ss;
    std::visit(Overloaded{
                       [&](DoctypeToken const &t) {
                           ss << "Doctype";
                           try_print(ss, t.name);
                           try_print(ss, t.public_identifier);
                           try_print(ss, t.system_identifier);
                       },
                       [&ss](StartTagToken const &t) { ss << "StartTag " << t.tag_name << ' ' << t.self_closing; },
                       [&ss](EndTagToken const &t) { ss << "EndTag " << t.tag_name << ' ' << t.self_closing; },
                       [&ss](CommentToken const &t) { ss << "Comment " << t.data; },
                       [&ss](CharacterToken const &t) { ss << "Character " << t.data; },
                       [&ss](EndOfFileToken const &) { ss << "EndOfFile"; },
               },
            token);
    return ss.str();
}

void Tokenizer::set_state(State state) {
    state_ = state;
}

void Tokenizer::run() {
    while (true) {
        if (input_.size() > pos_) {
            spdlog::trace("Running state {} w/ next char {}", state_, input_[pos_]);
        } else {
            spdlog::trace("Running state {} after input end", state_);
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
                    append_to_current_attribute_name(*c);
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

            default:
                std::terminate();
        }
    }
}

void Tokenizer::emit(Token &&token) {
    on_emit_(std::move(token), *this);
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

} // namespace html2
