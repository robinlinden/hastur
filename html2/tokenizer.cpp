// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "html2/tokenizer.h"

#include <spdlog/spdlog.h>

#include <cctype>
#include <cstring>
#include <exception>

using namespace std::literals;

namespace html2 {
namespace {

constexpr bool no_case_compare(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) {
        return false;
    }

    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(a[i]) != std::tolower(b[i])) {
            return false;
        }
    }

    return true;
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

constexpr char to_lower(char c) {
    return c + 0x20;
}

} // namespace

void Tokenizer::run() {
    while (!is_eof()) {
        spdlog::trace("Running state {} w/ next char {}", state_, input_[pos_]);
        switch (state_) {
            case State::Data: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(EndOfFileToken{});
                    continue;
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
                    continue;
                }

                if (is_ascii_alpha(*c)) {
                    current_token_ = StartTagToken{};
                    --pos_;
                    state_ = State::TagName;
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
                    continue;
                }

                if (is_ascii_alpha(*c)) {
                    current_token_ = EndTagToken{};
                    --pos_;
                    state_ = State::TagName;
                    continue;
                }

                std::terminate();
            }

            case State::TagName: {
                auto c = consume_next_input_character();
                if (!c) {
                    // This is an eof-in-tag parse error.
                    emit(EndOfFileToken{});
                    continue;
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

            case State::MarkupDeclarationOpen:
                if (no_case_compare(input_.substr(pos_, std::strlen("DOCTYPE")), "doctype"sv)) {
                    pos_ += std::strlen("DOCTYPE");
                    state_ = State::Doctype;
                    continue;
                }
                break;

            case State::Doctype: {
                auto c = consume_next_input_character();
                if (!c) {
                    // This is an eof-in-doctype parse error.
                    emit(DoctypeToken{.force_quirks = true});
                    emit(EndOfFileToken{});
                    continue;
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
                    continue;
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

            default:
                std::terminate();
        }
    }
}

void Tokenizer::emit(Token &&token) const {
    on_emit_(std::move(token));
}

std::optional<char> Tokenizer::consume_next_input_character() {
    if (is_eof()) {
        return std::nullopt;
    }

    return input_[pos_++];
}

bool Tokenizer::is_eof() const {
    return pos_ >= input_.size();
}

} // namespace html2
