// SPDX-FileCopyrightText: 2021-2024 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "css2/tokenizer.h"

#include "css2/token.h"

#include "unicode/util.h"
#include "util/from_chars.h"
#include "util/string.h"

#include <cassert>
#include <charconv>
#include <cstdint>
#include <optional>
#include <string>
#include <system_error>
#include <tuple>
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
                        if (inputs_starts_number(*c)) {
                            auto number = consume_number(*c);
                            emit(NumberToken{number});
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
                        if (inputs_starts_number(*c)) {
                            auto number = consume_number(*c);
                            emit(NumberToken{number});
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
                        emit(NumberToken{number});
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
                }

                if (*c == '\\') {
                    temporary_buffer_ += consume_an_escaped_code_point();
                    continue;
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
                }

                if (*c == '\\') {
                    temporary_buffer_ += consume_an_escaped_code_point();
                    continue;
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
                        std::get<StringToken>(current_token_).data += consume_an_escaped_code_point();
                        continue;
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

bool Tokenizer::inputs_starts_number(char first_character) const {
    assert(first_character == '-' || first_character == '+');

    if (auto next_input = peek_input(0); next_input && util::is_digit(*next_input)) {
        return true;
    }

    return false;
}

bool Tokenizer::is_eof() const {
    return pos_ >= input_.size();
}

void Tokenizer::reconsume_in(State state) {
    --pos_;
    state_ = state;
}

// https://www.w3.org/TR/css-syntax-3/#consume-a-number
std::variant<int, double> Tokenizer::consume_number(char first_byte) {
    std::variant<int, double> result{};
    std::string repr{};

    assert(util::is_digit(first_byte) || first_byte == '-' || first_byte == '+');
    if (first_byte != '+') {
        repr += first_byte;
    }

    for (auto next_input = peek_input(0); next_input && util::is_digit(*next_input); next_input = peek_input(0)) {
        repr += *next_input;
        consume_next_input_character();
    }

    if (peek_input(0) == '.' && util::is_digit(peek_input(1).value_or('Q'))) {
        std::ignore = consume_next_input_character(); // '.'
        auto v = consume_next_input_character();
        assert(v.has_value());
        repr += '.';
        repr += *v;
        result = 0.;

        for (auto next_input = peek_input(0); next_input && util::is_digit(*next_input); next_input = peek_input(0)) {
            repr += *next_input;
            consume_next_input_character();
        }
    }

    // TODO(robinlinden): Step 5

    // The tokenizer will verify that this is a number before calling consume_number.
    [[maybe_unused]] util::from_chars_result fc_res{};
    if (auto *int_res = std::get_if<int>(&result); int_res != nullptr) {
        fc_res = util::from_chars(repr.data(), repr.data() + repr.size(), *int_res);
    } else {
        fc_res = util::from_chars(repr.data(), repr.data() + repr.size(), std::get<double>(result));
    }

    assert(fc_res.ec == std::errc{} && fc_res.ptr == repr.data() + repr.size());
    return result;
}

// https://www.w3.org/TR/css-syntax-3/#consume-escaped-code-point
std::string Tokenizer::consume_an_escaped_code_point() {
    static constexpr std::uint32_t kReplacementCharacter = 0xFFFD;
    auto c = consume_next_input_character();
    if (!c) {
        emit(ParseError::EofInEscapeSequence);
        return unicode::to_utf8(kReplacementCharacter);
    }

    if (util::is_hex_digit(*c)) {
        std::string hex{*c};
        for (int i = 0; i < 5; ++i) {
            auto next_input = peek_input(0);
            if (!next_input || !util::is_hex_digit(*next_input)) {
                break;
            }

            hex += *next_input;
            std::ignore = consume_next_input_character();
        }

        if (auto next_input = peek_input(0); next_input && util::is_whitespace(*next_input)) {
            std::ignore = consume_next_input_character();
        }

        std::uint32_t code_point{};
        [[maybe_unused]] auto res = std::from_chars(hex.data(), hex.data() + hex.size(), code_point, 16);
        assert(res.ec == std::errc{} && res.ptr == hex.data() + hex.size());

        // https://www.w3.org/TR/css-syntax-3/#maximum-allowed-code-point
        static constexpr std::uint32_t kMaximumAllowedCodePoint = 0x10FFFF;
        if (code_point == 0 || code_point > kMaximumAllowedCodePoint || unicode::is_surrogate(code_point)) {
            code_point = kReplacementCharacter;
        }

        return unicode::to_utf8(code_point);
    }

    return std::string{*c};
}

} // namespace css2
