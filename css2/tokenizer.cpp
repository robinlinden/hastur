// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "css2/tokenizer.h"

#include "css2/token.h"

#include "util/string.h"

#include <cassert>
#include <charconv>
#include <exception>
#include <optional>
#include <string>
#include <system_error>
#include <utility>
#include <variant>

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
                    case '@':
                        state_ = State::CommercialAt;
                        continue;
                    case '(':
                        emit(OpenParenToken{});
                        continue;
                    case ')':
                        emit(CloseParenToken{});
                        continue;
                    case '+': {
                        // TODO(robinlinden): This only handles integers.
                        if (auto next_input = peek_input(0); next_input && util::is_digit(*next_input)) {
                            auto number = consume_number(*c);
                            emit(NumberToken{number.second, number.first});
                        } else {
                            emit(DelimToken{'+'});
                        }
                        continue;
                    }
                    case ',':
                        emit(CommaToken{});
                        continue;
                    case '-': {
                        // TODO(robinlinden): This only handles integers.
                        if (auto next_input = peek_input(0); next_input && util::is_digit(*next_input)) {
                            auto number = consume_number(*c);
                            emit(NumberToken{number.second, number.first});
                            continue;
                        }

                        if (peek_input(0) == '-' && peek_input(1) == '>') {
                            emit(CdcToken{});
                            pos_ += 2;
                            continue;
                        }

                        if (inputs_starts_ident_sequence(*c)) {
                            reconsume_in(State::IdentLike);
                            continue;
                        }

                        emit(DelimToken{'-'});
                        continue;
                    }
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
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9': {
                        // TODO(robinlinden): https://www.w3.org/TR/css-syntax-3/#consume-a-numeric-token
                        auto number = consume_number(*c);
                        emit(NumberToken{number.second, number.first});
                        continue;
                    }
                    default:
                        break;
                }

                if (inputs_starts_ident_sequence(*c)) {
                    temporary_buffer_ = *c;
                    state_ = State::IdentLike;
                    continue;
                }

                emit(DelimToken{*c});
                continue;
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

            case State::CommercialAt: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(DelimToken{'@'});
                    return;
                }

                if (inputs_starts_ident_sequence(*c)) {
                    temporary_buffer_ = *c;
                    state_ = State::CommercialAtIdent;
                    continue;
                }

                emit(DelimToken{'@'});
                reconsume_in(State::Main);
                continue;
            }

            case State::CommercialAtIdent: {
                auto c = consume_next_input_character();
                if (!c) {
                    emit(AtKeywordToken{temporary_buffer_});
                    return;
                }

                if (is_ident_code_point(*c)) {
                    temporary_buffer_ += *c;
                    continue;
                } else if (*c == '\\') {
                    // TODO(mkiael): Handle escaped code point
                    std::terminate();
                }

                emit(AtKeywordToken{temporary_buffer_});
                reconsume_in(State::Main);
                continue;
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

std::optional<char> Tokenizer::peek_input(int index) const {
    if (pos_ + index >= input_.size()) {
        return std::nullopt;
    }

    return input_[pos_ + index];
}

// https://www.w3.org/TR/css-syntax-3/#would-start-an-identifier
bool Tokenizer::inputs_starts_ident_sequence(char first_character) const {
    bool result{false};
    if (first_character == '-') {
        if (auto second_character = peek_input(0)) {
            if (is_ident_start_code_point(*second_character) || *second_character == '-') {
                result = true;
            }
        }
    } else if (is_ident_start_code_point(first_character)) {
        result = true;
    }
    // TODO(mkiael): Handle escape sequence
    return result;
}

bool Tokenizer::is_eof() const {
    return pos_ >= input_.size();
}

void Tokenizer::reconsume_in(State state) {
    --pos_;
    state_ = state;
}

// https://www.w3.org/TR/css-syntax-3/#consume-a-number
std::pair<std::variant<int, double>, NumericType> Tokenizer::consume_number(char first_byte) {
    NumericType type{NumericType::Integer};
    std::variant<int, double> result{};
    std::string repr{};

    std::optional<char> next_input;
    if (first_byte == '-') {
        repr += first_byte;
        next_input = consume_next_input_character();
    } else if (first_byte == '+') {
        next_input = consume_next_input_character();
    } else {
        next_input = first_byte;
    }

    for (; next_input && util::is_digit(*next_input); next_input = consume_next_input_character()) {
        repr += *next_input;
    }

    // TODO(robinlinden): Step 4, 5

    [[maybe_unused]] auto fc_res = std::from_chars(repr.data(), repr.data() + repr.size(), std::get<int>(result));
    // The tokenizer will verify that this is a number before calling consume_number.
    assert(fc_res.ec == std::errc{});

    return {result, type};
}

} // namespace css2
