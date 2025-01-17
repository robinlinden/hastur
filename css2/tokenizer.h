// SPDX-FileCopyrightText: 2021-2025 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef CSS2_TOKENIZER_H_
#define CSS2_TOKENIZER_H_

#include "css2/token.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace css2 {

// https://www.w3.org/TR/css-syntax-3/#tokenizer-algorithms
enum class State : std::uint8_t {
    Main,
    CommentStart,
    Comment,
    CommentEnd,
    CommercialAt,
    CommercialAtIdent,
    IdentLike,
    String,
    Whitespace,
};

enum class ParseError : std::uint8_t {
    EofInComment,
    EofInEscapeSequence,
    EofInString,
    NewlineInString,
};

std::string_view to_string(ParseError);

class Tokenizer {
public:
    Tokenizer(std::string_view input, std::function<void(Token &&)> on_emit, std::function<void(ParseError)> on_error)
        : input_(input), on_emit_(std::move(on_emit)), on_error_(std::move(on_error)) {}

    void run();

private:
    std::string_view input_;
    std::size_t pos_{0};
    State state_{State::Main};
    Token current_token_{};

    char string_ending_{};

    std::string temporary_buffer_{};

    std::function<void(Token &&)> on_emit_;
    std::function<void(ParseError)> on_error_;

    void emit(ParseError);
    void emit(Token &&);
    std::optional<char> consume_next_input_character();
    std::optional<char> peek_input(int index) const;
    bool inputs_starts_ident_sequence(char first_character) const;
    bool inputs_starts_number(char first_character) const;
    bool is_eof() const;
    void reconsume();
    void reconsume_in(State);

    std::variant<std::int32_t, double> consume_number(char first_byte);
    std::string consume_an_escaped_code_point();
    Token consume_a_numeric_token(char first_byte);
    [[nodiscard]] std::string consume_an_ident_sequence(char first_byte);
};

} // namespace css2

#endif
