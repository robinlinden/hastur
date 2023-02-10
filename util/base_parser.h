// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2022 David Zero <zero-one@zer0-one.net>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef UTIL_BASE_PARSER_H_
#define UTIL_BASE_PARSER_H_

#include "util/string.h"

#include <concepts>
#include <cstddef>
#include <string_view>

namespace util {

template<typename T>
concept Predicate = std::predicate<T, char>;

class BaseParser {
public:
    constexpr explicit BaseParser(std::string_view input) : input_{input} {}

    constexpr char peek() const { return input_[pos_]; }

    constexpr std::string_view peek(std::size_t chars) const { return input_.substr(pos_, chars); }

    constexpr bool starts_with(std::string_view prefix) const { return peek(prefix.size()) == prefix; }

    constexpr bool is_eof() const { return pos_ >= input_.size(); }

    constexpr char consume_char() { return input_[pos_++]; }

    constexpr void advance(std::size_t n) { pos_ += n; }

    constexpr void back(std::size_t n) { pos_ -= n; }

    constexpr void reset() { pos_ = 0; }

    constexpr void reset(std::string_view input) {
        input_ = input;
        pos_ = 0;
    }

    template<Predicate T>
    constexpr std::string_view consume_while(T const &pred) {
        std::size_t start = pos_;
        while (pred(input_[pos_])) {
            ++pos_;
        }
        return input_.substr(start, pos_ - start);
    }

    constexpr void skip_whitespace() {
        while (!is_eof() && util::is_whitespace(peek())) {
            advance(1);
        }
    }

private:
    std::string_view input_;
    std::size_t pos_{0};
};

} // namespace util

#endif
