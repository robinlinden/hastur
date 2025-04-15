// SPDX-FileCopyrightText: 2021-2025 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "css2/tokenizer.h"

#include "css2/token.h"

#include "unicode/util.h"
#include "util/from_chars.h"
#include "util/string.h"

#include <algorithm>
#include <cassert>
#include <charconv>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
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

constexpr bool is_digit(std::optional<char> c) {
    return c && util::is_digit(*c);
}

// https://www.w3.org/TR/css-syntax-3/#check-if-two-code-points-are-a-valid-escape
constexpr bool is_valid_escape_sequence(char first_byte, std::optional<char> second_byte) {
    return first_byte == '\\' && second_byte != '\n';
}

constexpr bool is_whitespace(char c) {
    return c == ' ' || c == '\n' || c == '\t';
}

constexpr bool is_whitespace(std::optional<char> c) {
    return c && is_whitespace(*c);
}

// https://www.w3.org/TR/css-syntax-3/#non-printable-code-point
constexpr bool is_non_printable(char c) {
    return (c >= 0x00 && c <= 0x08) || c == 0x0B || (c >= 0x0E && c <= 0x1F) || c == 0x7F;
}

} // namespace

std::string_view to_string(ParseError e) {
    switch (e) {
        case ParseError::DisallowedCharacterInUrl:
            return "DisallowedCharacterInUrl";
        case ParseError::EofInComment:
            return "EofInComment";
        case ParseError::EofInEscapeSequence:
            return "EofInEscapeSequence";
        case ParseError::EofInString:
            return "EofInString";
        case ParseError::EofInUrl:
            return "EofInUrl";
        case ParseError::InvalidEscapeSequence:
            return "InvalidEscapeSequence";
        case ParseError::NewlineInString:
            return "NewlineInString";
    }

    return "Unknown parse error";
}

// https://www.w3.org/TR/css-syntax-3/#tokenizer-algorithms
void Tokenizer::run() {
    while (true) {
        consume_comments();

        auto c = consume_next_input_character();
        if (!c) {
            return;
        }

        if (is_whitespace(*c)) {
            while (is_whitespace(consume_next_input_character())) {
                // Do nothing.
            }

            reconsume();
            emit(WhitespaceToken{});
            continue;
        }

        switch (*c) {
            case '\'':
            case '"':
                emit(consume_string(*c));
                continue;
            case '#': {
                auto next_input = peek_input(0);
                if (!next_input) {
                    emit(DelimToken{'#'});
                    continue;
                }

                if (is_ident_code_point(*next_input) || is_valid_escape_sequence(*next_input, peek_input(1))) {
                    std::ignore = consume_next_input_character();
                    HashToken token{};

                    if (inputs_starts_ident_sequence(*next_input)) {
                        token.type = HashToken::Type::Id;
                    }

                    token.data = consume_an_ident_sequence(*next_input);
                    emit(std::move(token));
                    continue;
                }

                emit(DelimToken{'#'});
                continue;
            }
            case '@': {
                auto next_input = consume_next_input_character();
                if (!next_input || !inputs_starts_ident_sequence(*next_input)) {
                    reconsume();
                    emit(DelimToken{'@'});
                    continue;
                }

                emit(AtKeywordToken{.data = consume_an_ident_sequence(*next_input)});
                continue;
            }
            case '(':
                emit(OpenParenToken{});
                continue;
            case ')':
                emit(CloseParenToken{});
                continue;
            case '+': {
                if (inputs_starts_number(*c)) {
                    emit(consume_a_numeric_token(*c));
                } else {
                    emit(DelimToken{'+'});
                }
                continue;
            }
            case ',':
                emit(CommaToken{});
                continue;
            case '-': {
                if (inputs_starts_number(*c)) {
                    emit(consume_a_numeric_token(*c));
                    continue;
                }

                if (peek_input(0) == '-' && peek_input(1) == '>') {
                    emit(CdcToken{});
                    pos_ += 2;
                    continue;
                }

                if (inputs_starts_ident_sequence(*c)) {
                    emit(consume_an_identlike_token(*c));
                    continue;
                }

                emit(DelimToken{'-'});
                continue;
            }
            case '.': {
                if (auto next_input = peek_input(0); is_digit(next_input)) {
                    emit(consume_a_numeric_token(*c));
                    continue;
                }

                emit(DelimToken{'.'});
                continue;
            }
            case ':':
                emit(ColonToken{});
                continue;
            case ';':
                emit(SemiColonToken{});
                continue;
            case '<':
                if (peek_input(0) == '!' && peek_input(1) == '-' && peek_input(2) == '-') {
                    emit(CdoToken{});
                    pos_ += 3;
                    continue;
                }

                emit(DelimToken{'<'});
                continue;
            case '[':
                emit(OpenSquareToken{});
                continue;
            case '\\':
                if (is_valid_escape_sequence('\\', peek_input(0))) {
                    emit(consume_an_identlike_token(*c));
                    continue;
                }

                emit(ParseError::InvalidEscapeSequence);
                emit(DelimToken{'\\'});
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
                emit(consume_a_numeric_token(*c));
                continue;
            }
            default:
                break;
        }

        if (is_ident_start_code_point(*c)) {
            emit(consume_an_identlike_token(*c));
            continue;
        }

        emit(DelimToken{*c});
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
    if (first_character == '-') {
        auto second_character = peek_input(0);
        if (!second_character) {
            return false;
        }

        if (is_ident_start_code_point(*second_character) || *second_character == '-') {
            return true;
        }

        auto third_character = peek_input(1);
        return is_valid_escape_sequence(*second_character, third_character);
    }

    if (is_ident_start_code_point(first_character)) {
        return true;
    }

    return is_valid_escape_sequence(first_character, peek_input(0));
}

bool Tokenizer::inputs_starts_number([[maybe_unused]] char first_character) const {
    assert(first_character == '-' || first_character == '+');

    auto next_input = peek_input(0);
    if (!next_input) {
        return false;
    }

    if (util::is_digit(*next_input)) {
        return true;
    }

    auto next_next_input = peek_input(1);
    if (!next_next_input) {
        return false;
    }

    return next_input == '.' && util::is_digit(*next_next_input);
}

bool Tokenizer::is_eof() const {
    return pos_ >= input_.size();
}

void Tokenizer::reconsume() {
    --pos_;
}

Token Tokenizer::consume_string(char ending_code_point) {
    std::string result{};

    while (true) {
        auto c = consume_next_input_character();

        if (!c) {
            emit(ParseError::EofInString);
            return StringToken{std::move(result)};
        }

        if (*c == ending_code_point) {
            return StringToken{std::move(result)};
        }

        if (*c == '\n') {
            emit(ParseError::NewlineInString);
            reconsume();
            return BadStringToken{};
        }

        if (*c == '\\') {
            if (is_eof()) {
                continue;
            }

            if (peek_input(0) == '\n') {
                std::ignore = consume_next_input_character();
                continue;
            }

            result += consume_an_escaped_code_point();
            continue;
        }

        result += *c;
    }
}

// https://www.w3.org/TR/css-syntax-3/#consume-a-number
std::variant<std::int32_t, double> Tokenizer::consume_number(char first_byte) {
    std::variant<std::int32_t, double> result{};
    std::string repr{};

    assert(util::is_digit(first_byte) || first_byte == '-' || first_byte == '+' || first_byte == '.');

    if (first_byte == '.') {
        repr += "0.";
        result = 0.;
    } else if (first_byte != '+') {
        repr += first_byte;
    }

    for (auto next_input = peek_input(0); is_digit(next_input); next_input = peek_input(0)) {
        assert(next_input); // Guaranteed by is_digit.
        repr += *next_input;
        consume_next_input_character();
    }

    if (!std::holds_alternative<double>(result) && peek_input(0) == '.' && is_digit(peek_input(1))) {
        std::ignore = consume_next_input_character(); // '.'
        auto v = consume_next_input_character();
        assert(v.has_value());
        repr += '.';
        repr += *v;
        result = 0.;

        for (auto next_input = peek_input(0); is_digit(next_input); next_input = peek_input(0)) {
            assert(next_input); // Guaranteed by is_digit.
            repr += *next_input;
            consume_next_input_character();
        }
    }

    bool const has_e_notation = [&] {
        if (auto c = peek_input(0); c != 'e' && c != 'E') {
            return false;
        }

        if (auto c = peek_input(1); c == '+' || c == '-') {
            return is_digit(peek_input(2));
        }

        return is_digit(peek_input(1));
    }();

    if (has_e_notation) {
        std::ignore = consume_next_input_character(); // 'e' or 'E'
        repr += 'e';
        auto c = consume_next_input_character(); // '+', '-', or a number.
        assert(c.has_value()); // Guaranteed by has_e_notation.
        repr += *c;

        result = 0.;

        for (auto next_input = peek_input(0); is_digit(next_input); next_input = peek_input(0)) {
            assert(next_input); // Guaranteed by is_digit.
            repr += *next_input;
            consume_next_input_character();
        }
    }

    // The tokenizer will verify that this is a number before calling consume_number.
    //
    // The spec doesn't mention precision of this, so let's clamp it to the
    // int32_t range for now.
    util::from_chars_result fc_res{};
    if (auto *int_res = std::get_if<std::int32_t>(&result); int_res != nullptr) {
        fc_res = util::from_chars(repr.data(), repr.data() + repr.size(), *int_res);
        *int_res = std::clamp(
                *int_res, std::numeric_limits<std::int32_t>::min(), std::numeric_limits<std::int32_t>::max());
    } else {
        auto &dbl_res = std::get<double>(result);
        fc_res = util::from_chars(repr.data(), repr.data() + repr.size(), dbl_res);
        dbl_res = std::clamp(dbl_res,
                static_cast<double>(std::numeric_limits<std::int32_t>::min()),
                static_cast<double>(std::numeric_limits<std::int32_t>::max()));
    }

    if (fc_res.ec == std::errc::result_out_of_range) {
        result = repr[0] == '-' ? std::numeric_limits<std::int32_t>::min() : std::numeric_limits<std::int32_t>::max();
    } else {
        assert(fc_res.ec == std::errc{} && fc_res.ptr == repr.data() + repr.size());
    }

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

// https://www.w3.org/TR/css-syntax-3/#consume-a-numeric-token
Token Tokenizer::consume_a_numeric_token(char first_byte) {
    auto number = consume_number(first_byte);
    auto next_input = consume_next_input_character();
    if (!next_input) {
        return NumberToken{number};
    }

    if (inputs_starts_ident_sequence(*next_input)) {
        return DimensionToken{.data = number, .unit = consume_an_ident_sequence(*next_input)};
    }

    if (*next_input == '%') {
        return PercentageToken{number};
    }

    reconsume();
    return NumberToken{number};
}

// https://www.w3.org/TR/css-syntax-3/#consume-name
std::string Tokenizer::consume_an_ident_sequence(char first_byte) {
    std::string result{};
    for (std::optional<char> c = first_byte; c.has_value(); c = consume_next_input_character()) {
        if (is_ident_code_point(*c)) {
            result += *c;
            continue;
        }

        if (is_valid_escape_sequence(*c, peek_input(0))) {
            result += consume_an_escaped_code_point();
            continue;
        }

        reconsume();
        break;
    }

    return result;
}

// https://www.w3.org/TR/css-syntax-3/#consume-an-ident-like-token
Token Tokenizer::consume_an_identlike_token(char first_byte) {
    auto ident = consume_an_ident_sequence(first_byte);

    if (util::no_case_compare(ident, "url") && peek_input(0) == '(') {
        std::ignore = consume_next_input_character(); // '('
        while (is_whitespace(peek_input(0)) && is_whitespace(peek_input(1))) {
            std::ignore = consume_next_input_character(); // whitespace
        }

        if ((peek_input(0) == '\'' || peek_input(0) == '"')
                || (is_whitespace(peek_input(0)) && (peek_input(1) == '\'' || peek_input(1) == '"'))) {
            return FunctionToken{std::move(ident)};
        }

        return consume_a_url_token();
    }

    if (peek_input(0) == '(') {
        std::ignore = consume_next_input_character(); // '('
        return FunctionToken{std::move(ident)};
    }

    return IdentToken{std::move(ident)};
}

// https://www.w3.org/TR/css-syntax-3/#consume-a-url-token
Token Tokenizer::consume_a_url_token() {
    while (is_whitespace(peek_input(0))) {
        std::ignore = consume_next_input_character();
    }

    std::string url{};

    while (true) {
        auto c = consume_next_input_character();
        if (!c) {
            emit(ParseError::EofInUrl);
            return UrlToken{std::move(url)};
        }

        if (*c == ')') {
            return UrlToken{std::move(url)};
        }

        if (is_whitespace(*c)) {
            while (is_whitespace(peek_input(0))) {
                std::ignore = consume_next_input_character();
            }

            if (peek_input(0) == ')') {
                std::ignore = consume_next_input_character();
                return UrlToken{std::move(url)};
            }

            if (peek_input(0) == std::nullopt) {
                emit(ParseError::EofInUrl);
                return UrlToken{std::move(url)};
            }

            consume_the_remnants_of_a_bad_url();
            return BadUrlToken{};
        }

        if (*c == '"' || *c == '\'' || *c == '(' || is_non_printable(*c)) {
            emit(ParseError::DisallowedCharacterInUrl);
            consume_the_remnants_of_a_bad_url();
            return BadUrlToken{};
        }

        if (*c == '\\') {
            if (is_valid_escape_sequence(*c, peek_input(0))) {
                url += consume_an_escaped_code_point();
                continue;
            }

            emit(ParseError::InvalidEscapeSequence);
            consume_the_remnants_of_a_bad_url();
            return BadUrlToken{};
        }

        url += *c;
    }
}

// https://www.w3.org/TR/css-syntax-3/#consume-the-remnants-of-a-bad-url
void Tokenizer::consume_the_remnants_of_a_bad_url() {
    while (true) {
        auto c = consume_next_input_character();
        if (!c || *c == ')') {
            return;
        }

        if (is_valid_escape_sequence(*c, peek_input(0))) {
            std::ignore = consume_an_escaped_code_point();
        }
    }
}

void Tokenizer::consume_comments() {
    while (peek_input(0) == '/' && peek_input(1) == '*') {
        std::ignore = consume_next_input_character(); // '/'
        std::ignore = consume_next_input_character(); // '*'

        while (true) {
            auto c = consume_next_input_character();
            if (!c) {
                emit(ParseError::EofInComment);
                return;
            }

            if (*c == '*' && peek_input(0) == '/') {
                std::ignore = consume_next_input_character();
                break;
            }
        }
    }
}

} // namespace css2
