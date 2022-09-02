// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef CSS2_TOKENIZER_H_
#define CSS2_TOKENIZER_H_

#include "css2/token.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <string_view>

namespace css2 {

// https://www.w3.org/TR/css-syntax-3/#tokenizer-algorithms
enum class State {
    Main,
    CommentStart,
    Comment,
    CommentEnd,
};

enum class ParseError {
    EofInComment,
};

class Tokenizer {
public:
    Tokenizer(std::string_view input, std::function<void(Token &&)> on_emit, std::function<void(ParseError)> on_error)
        : input_(input), on_emit_(on_emit), on_error_(on_error) {}

    void run();

private:
    std::string_view input_;
    std::size_t pos_{0};
    State state_{State::Main};

    std::function<void(Token &&)> on_emit_;
    std::function<void(ParseError)> on_error_;

    void emit(ParseError);
    void emit(Token &&);
    std::optional<char> consume_next_input_character();
    bool is_eof() const;
    void reconsume_in(State);
};

} // namespace css2

#endif
