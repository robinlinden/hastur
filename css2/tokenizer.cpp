// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "css2/tokenizer.h"

#include "util/string.h"

#include <exception>

namespace css2 {

namespace {

constexpr bool is_ident_start_code_point(char c) {
    // TODO(mkiael): Handle non-ascii code point
    return util::is_alpha(c) || c == '_';
}

constexpr bool is_ident_code_point(char c) {
    return is_ident_start_code_point(c) || util::is_digit(c) || c == '-';
}

} // namespace

void Tokenizer::run() {
    while (true) {
        switch (state_) {
            case State::Main: {
                auto c = consume_next_input_character();
                if (!c) {
                    return;
                }

                if (is_ident_start_code_point(*c)) {
                    temporary_buffer_ = "";
                    reconsume_in(State::IdentLike);
                    continue;
                }

                switch (*c) {
                    case ' ':
                    case '\n':
                    case '\t':
                        state_ = State::Whitespace;
                        continue;
                    case '\'':
                    case '"':
                        string_ending_ = *c;
                        current_token_ = StringToken{""};
                        state_ = State::String;
                        continue;
                    case '/':
                        state_ = State::CommentStart;
                        continue;
                    case '-':
                        state_ = State::HyphenMinus;
                        continue;
                    case '(':
                        emit(OpenParenToken{});
                        continue;
                    case ')':
                        emit(CloseParenToken{});
                        continue;
                    case ',':
                        emit(CommaToken{});
                        continue;
                    case ':':
                        emit(ColonToken{});
                        continue;
                    case ';':
                        emit(SemiColonToken{});
                        continue;
                    case '[':
                        emit(OpenSquareToken{});
                        continue;
                    case ']':
                        emit(CloseSquareToken{});
                        continue;
                    case '{':
                        emit(OpenCurlyToken{});
                        continue;
                    case '}':
                        emit(CloseCurlyToken{});
                        continue;
                    default:
                        emit(DelimToken{*c});
                        continue;
                }
            }

            case State::CommentStart: {
                auto c = consume_next_input_character();
                if (!c) {
                    return;
                }

                if (*c == '*') {
                    state_ = State::Comment;
                } else {
                    emit(DelimToken{'/'});
                    reconsume_in(State::Main);
                }
                continue;
            }

            case State::Comment: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::EofInComment);
                    return;
                }

                if (*c == '*') {
                    state_ = State::CommentEnd;
                }
                continue;
            }

            case State::CommentEnd: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::EofInComment);
                    return;
                }

                switch (*c) {
                    case '*':
                        continue;
                    case '/':
                        state_ = State::Main;
                        continue;
                    default:
                        state_ = State::Comment;
                        continue;
                }
            }

            case State::HyphenMinus: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(DelimToken{'-'});
                    return;
                }

                if (is_ident_start_code_point(*c) || *c == '-') {
                    temporary_buffer_ = '-';
                    temporary_buffer_ += *c;
                    state_ = State::IdentLike;
                    continue;
                }

                // TODO(mkiael): Handle numeric token
                // TODO(mkiael): Handle escaped code point in ident sequence
                std::terminate();
            }

            case State::IdentLike: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(IdentToken{temporary_buffer_});
                    return;
                }

                if (is_ident_code_point(*c)) {
                    temporary_buffer_ += *c;
                    continue;
                } else if (*c == '\\') {
                    // TODO(mkiael): Handle escaped code point
                    std::terminate();
                }

                // TODO(mkiael): Handle url and function token

                emit(IdentToken{temporary_buffer_});
                reconsume_in(State::Main);
                continue;
            }

            case State::String: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(ParseError::EofInString);
                    emit(std::move(current_token_));
                    return;
                }

                if (*c == string_ending_) {
                    emit(std::move(current_token_));
                    state_ = State::Main;
                    continue;
                }

                switch (*c) {
                    case '\\':
                        // TODO(mkiael): Handle escaped code point
                        std::terminate();
                    case '\n':
                        emit(ParseError::NewlineInString);
                        emit(BadStringToken{});
                        reconsume_in(State::Main);
                        continue;
                    default:
                        std::get<StringToken>(current_token_).data.append(1, *c);
                        continue;
                }
            }

            case State::Whitespace: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(WhitespaceToken{});
                    return;
                }

                switch (*c) {
                    case ' ':
                    case '\n':
                    case '\t':
                        continue;
                    default:
                        emit(WhitespaceToken{});
                        reconsume_in(State::Main);
                        continue;
                }
            }
        }
    }
}

void Tokenizer::emit(ParseError e) {
    on_error_(e);
}

void Tokenizer::emit(Token &&token) {
    on_emit_(std::move(token));
}

std::optional<char> Tokenizer::consume_next_input_character() {
    if (is_eof()) {
        pos_ += 1;
        return std::nullopt;
    }

    return input_[pos_++];
}

bool Tokenizer::is_eof() const {
    return pos_ >= input_.size();
}

void Tokenizer::reconsume_in(State state) {
    --pos_;
    state_ = state;
}

} // namespace css2
