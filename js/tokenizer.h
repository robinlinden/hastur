// SPDX-FileCopyrightText: 2023-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef JS_TOKENIZER_H_
#define JS_TOKENIZER_H_

#include "js/token.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace js::parse {

class Tokenizer {
public:
    explicit Tokenizer(std::string_view input) : input_{input} {}

    std::optional<Token> tokenize() {
        std::optional<char> current = consume();

        while (is_whitespace(current)) {
            current = consume();
        }

        // Handle multi-line comments.
        if (current == '/' && peek() == '*') {
            pos_ += 1;

            Comment comment{};
            while (true) {
                current = consume();
                if (!current) {
                    return comment;
                }

                if (*current == '*' && peek() == '/') {
                    pos_ += 1;
                    return comment;
                }

                comment.comment += *current;
            }
        }

        if (!current) {
            return Eof{};
        }

        switch (*current) {
            case '(':
                return LParen{};
            case ')':
                return RParen{};
            case '{':
                return LBrace{};
            case '}':
                return RBrace{};
            case '[':
                return LBracket{};
            case ']':
                return RBracket{};
            case ';':
                return Semicolon{};
            case ',':
                return Comma{};
            case '.':
                return Period{};
            case '=':
                return Equals{};
            case '+':
                return Plus{};
            case '*':
                return Asterisk{};
            case '\'':
            case '"': {
                return tokenize_string_literal(*current);
            }
            default:
                break;
        }

        if (is_numeric(*current)) {
            return tokenize_int_literal(*current);
        }

        if (!is_ident_start(*current)) {
            return std::nullopt;
        }

        auto current_word = consume_word(*current);

        // The factory nonsense is to allow this to be constexpr even though
        // Token can contain non-constexpr data.
        using FactoryFn = Token (*)();
        static constexpr auto kReservedWords = std::to_array<std::pair<std::string_view, FactoryFn>>({
                {"await", make<Await>},
                {"break", make<Break>},
                {"case", make<Case>},
                {"catch", make<Catch>},
                {"class", make<Class>},
                {"const", make<Const>},
                {"continue", make<Continue>},
                {"debugger", make<Debugger>},
                {"default", make<Default>},
                {"delete", make<Delete>},
                {"do", make<Do>},
                {"else", make<Else>},
                {"enum", make<Enum>},
                {"export", make<Export>},
                {"extends", make<Extends>},
                {"false", make<False>},
                {"finally", make<Finally>},
                {"for", make<For>},
                {"function", make<Function>},
                {"if", make<If>},
                {"import", make<Import>},
                {"in", make<In>},
                {"instanceof", make<InstanceOf>},
                {"new", make<New>},
                {"null", make<Null>},
                {"return", make<Return>},
                {"super", make<Super>},
                {"switch", make<Switch>},
                {"this", make<This>},
                {"throw", make<Throw>},
                {"true", make<True>},
                {"try", make<Try>},
                {"typeof", make<TypeOf>},
                {"var", make<Var>},
                {"void", make<Void>},
                {"while", make<While>},
                {"with", make<With>},
                {"yield", make<Yield>},
        });

        if (auto it = std::ranges::find(kReservedWords, current_word, &decltype(kReservedWords)::value_type::first);
                it != end(kReservedWords)) {
            return it->second();
        }

        return Identifier{.name = std::move(current_word)};
    }

private:
    std::string_view input_;
    std::size_t pos_{};

    template<typename T>
    static Token make() {
        return T{};
    }

    std::optional<char> peek() const {
        if ((pos_) < input_.size()) {
            return input_[pos_];
        }
        return std::nullopt;
    }

    std::optional<char> consume() {
        if (pos_ < input_.size()) {
            return input_[pos_++];
        }

        return std::nullopt;
    }

    std::optional<Token> tokenize_int_literal(char current) {
        constexpr int kUpperBound = std::numeric_limits<int32_t>::max();

        std::uint64_t value{};
        while (true) {
            value += current - '0';
            if (std::cmp_greater(value, kUpperBound)) {
                return std::nullopt;
            }

            auto next = peek();
            if (!next || !is_numeric(*next)) {
                break;
            }
            value *= 10;
            current = *next;
            pos_ += 1;
        }

        assert(value <= kUpperBound);
        return IntLiteral{static_cast<int>(value)};
    }

    // https://tc39.es/ecma262/#prod-StringLiteral
    // TODO(robinlinden): More special cases.
    std::optional<Token> tokenize_string_literal(char quote) {
        auto token = std::make_optional<StringLiteral>();
        std::string &str = token->value;

        while (true) {
            auto current = consume();
            if (!current) {
                return std::nullopt;
            }

            if (*current == quote) {
                return token;
            }

            if (*current == '\\') {
                auto escaped = consume_escape_sequence();
                if (!escaped) {
                    return std::nullopt;
                }

                str += *escaped;
                continue;
            }

            str += *current;
        }
    }

    [[nodiscard]] std::optional<std::string> consume_escape_sequence() {
        auto current = consume();
        if (!current) {
            return std::nullopt;
        }

        // https://tc39.es/ecma262/#prod-SingleEscapeCharacter
        switch (*current) {
            case '\'':
                return "\'";
            case '"':
                return "\"";
            case '\\':
                return "\\";
            case 'b':
                return "\b";
            case 'f':
                return "\f";
            case 'n':
                return "\n";
            case 'r':
                return "\r";
            case 't':
                return "\t";
            case 'v':
                return "\v";
            default:
                return std::nullopt;
        }
    }

    [[nodiscard]] std::string consume_word(char current) {
        std::string word;
        while (true) {
            word += current;
            auto next = peek();
            if (!next || !is_ident_continuation(*next)) {
                break;
            }

            current = *next;
            pos_ += 1;
        }

        return word;
    }

    static constexpr bool is_alpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
    static constexpr bool is_numeric(char c) { return (c >= '0' && c <= '9'); }
    static constexpr bool is_ident_start(char c) { return is_alpha(c) || c == '_'; }
    static constexpr bool is_ident_continuation(char c) { return is_alpha(c) || is_numeric(c) || c == '_'; }
    static constexpr bool is_whitespace(std::optional<char> c) {
        if (!c) {
            return false;
        }

        switch (*c) {
            case ' ':
            case '\n':
            case '\r':
            case '\f':
            case '\v':
            case '\t':
                return true;
            default:
                return false;
        }
    }
};

inline std::optional<std::vector<Token>> tokenize(std::string_view input) {
    std::vector<Token> tokens;
    auto t = Tokenizer{input};

    while (true) {
        auto token = t.tokenize();
        if (!token) {
            return std::nullopt;
        }

        tokens.push_back(std::move(*token));
        if (std::holds_alternative<Eof>(tokens.back())) {
            return tokens;
        }
    }
}

} // namespace js::parse

#endif
