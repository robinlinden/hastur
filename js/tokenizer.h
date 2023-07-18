// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef JS_TOKENIZER_H_
#define JS_TOKENIZER_H_

#include <cassert>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace js::parse {

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

struct Eof {
    bool operator==(Eof const &) const = default;
};

using Token = std::variant< //
        Identifier,
        LParen,
        RParen,
        Semicolon,
        Eof>;

class Tokenizer {
public:
    explicit Tokenizer(std::string_view input) : input_{input} {}

    Token tokenize() {
        if (pos_ >= input_.size()) {
            return Eof{};
        }

        char current = input_[pos_++];
        switch (current) {
            case '(':
                return LParen{};
            case ')':
                return RParen{};
            case ';':
                return Semicolon{};
            default:
                break;
        }

        assert(is_alpha(current));
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

private:
    std::string_view input_;
    std::size_t pos_{};

    std::optional<char> peek() const {
        if ((pos_) < input_.size()) {
            return input_[pos_];
        }
        return std::nullopt;
    }

    static constexpr bool is_alpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
};

inline std::vector<Token> tokenize(std::string_view input) {
    std::vector<Token> tokens;
    auto t = Tokenizer{input};

    do {
        tokens.push_back(t.tokenize());
    } while (!std::holds_alternative<Eof>(tokens.back()));

    return tokens;
}

} // namespace js::parse

#endif
