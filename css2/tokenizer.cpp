// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "css2/tokenizer.h"

namespace css2 {

void Tokenizer::run() {
    while (true) {
        switch (state_) {
            case State::Main: {
                auto c = consume_next_input_character();
                if (!c) {
                    return;
                }

                switch (*c) {
                    case ' ':
                    case '\n':
                    case '\t':
                        state_ = State::Whitespace;
                        continue;
                    case '/':
                        state_ = State::CommentStart;
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
