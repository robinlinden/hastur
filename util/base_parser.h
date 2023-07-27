// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2022 David Zero <zero-one@zer0-one.net>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef UTIL_BASE_PARSER_H_
#define UTIL_BASE_PARSER_H_

#include "util/string.h"

#include <cstddef>
#include <optional>
#include <string_view>

namespace util {

class BaseParser {
public:
    constexpr explicit BaseParser(std::string_view input) : input_{input} {}

    constexpr std::optional<char> peek() const {
        if (is_eof()) {
            return std::nullopt;
        }

        return input_[pos_];
    }

    constexpr std::optional<std::string_view> peek(std::size_t chars) const {
        if (is_eof()) {
            return std::nullopt;
        }

        return input_.substr(pos_, chars);
    }

    constexpr std::string_view remaining_from(std::size_t skip) const {
        return pos_ + skip >= input_.size() ? "" : input_.substr(pos_ + skip);
    }

    constexpr bool is_eof() const { return pos_ >= input_.size(); }

    constexpr void advance(std::size_t n) { pos_ += n; }

    constexpr void back(std::size_t n) { pos_ -= n; }

    constexpr void reset() { pos_ = 0; }

    constexpr void reset(std::string_view input) {
        input_ = input;
        pos_ = 0;
    }

    constexpr std::size_t current_pos() const { return pos_; }

private:
    std::string_view input_;
    std::size_t pos_{0};
};

} // namespace util

#endif
