// SPDX-FileCopyrightText: 2023-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef JS_TOKENIZER_H_
#define JS_TOKENIZER_H_

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

struct IntLiteral {
    std::int32_t value{};
    bool operator==(IntLiteral const &) const = default;
};

struct Identifier {
    std::string name;
    bool operator==(Identifier const &) const = default;
};

struct LParen {
    bool operator==(LParen const &) const = default;
};

struct RParen {
    bool operator==(RParen const &) const = default;
};

struct Semicolon {
    bool operator==(Semicolon const &) const = default;
};

struct Comma {
    bool operator==(Comma const &) const = default;
};

struct Eof {
    bool operator==(Eof const &) const = default;
};

using Token = std::variant< //
        IntLiteral,
        Identifier,
        LParen,
        RParen,
        Semicolon,
        Comma,
        Eof>;

class Tokenizer {
public:
    explicit Tokenizer(std::string_view input) : input_{input} {}

    std::optional<Token> tokenize() {
        char current{};
        do {
            if (pos_ >= input_.size()) {
                return Eof{};
            }
            current = input_[pos_++];
        } while (is_whitespace(current));

        switch (current) {
            case '(':
                return LParen{};
            case ')':
                return RParen{};
            case ';':
                return Semicolon{};
            case ',':
                return Comma{};
            default:
                break;
        }

        if (is_numeric(current)) {
            return tokenize_int_literal(current);
        }

        if (!is_alpha(current)) {
            return std::nullopt;
        }

        return tokenize_identifier(current);
    }

private:
    std::string_view input_;
    std::size_t pos_{};

    std::optional<char> peek() const {
        if ((pos_) < input_.size()) {
            return input_[pos_];
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

    Token tokenize_identifier(char current) {
        Identifier id{};
        while (true) {
            id.name += current;
            auto next = peek();
            if (!next || !is_alpha(*next)) {
                break;
            }
            current = *next;
            pos_ += 1;
        }

        return id;
    }

    static constexpr bool is_alpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
    static constexpr bool is_numeric(char c) { return (c >= '0' && c <= '9'); }
    static constexpr bool is_whitespace(char c) {
        switch (c) {
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

    do {
        auto token = t.tokenize();
        if (!token) {
            return std::nullopt;
        }

        tokens.push_back(std::move(*token));
    } while (!std::holds_alternative<Eof>(tokens.back()));

    return tokens;
}

} // namespace js::parse

#endif
