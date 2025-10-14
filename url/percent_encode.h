// SPDX-FileCopyrightText: 2023 David Zero <zero-one@zer0-one.net>
// SPDX-FileCopyrightText: 2024-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef URL_PERCENT_ENCODE_H_
#define URL_PERCENT_ENCODE_H_

#include "util/string.h"

#include <cassert>
#include <charconv>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <format>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace url {

// https://url.spec.whatwg.org/#percent-encoded-bytes
struct PercentEncodeSet {
    static constexpr bool c0_control(char c) {
        return util::is_c0(c) || c == 0x7f || static_cast<std::uint8_t>(c) > 0x7f;
    }

    static constexpr bool fragment(char c) {
        return c0_control(c) || c == ' ' || c == '"' || c == '<' || c == '>' || c == '`';
    }

    static constexpr bool query(char c) {
        return c0_control(c) || c == ' ' || c == '"' || c == '#' || c == '<' || c == '>';
    }

    static constexpr bool special_query(char c) { return query(c) || c == '\''; }

    static constexpr bool path(char c) { return query(c) || c == '?' || c == '^' || c == '`' || c == '{' || c == '}'; }

    static constexpr bool userinfo(char c) {
        return path(c) || c == '/' || c == ':' || c == ';' || c == '=' || c == '@' || (c >= '[' && c <= ']')
                || c == '|';
    }

    static constexpr bool component(char c) { return userinfo(c) || (c >= '$' && c <= '&') || c == '+' || c == ','; }
};

// https://url.spec.whatwg.org/#string-percent-encode-after-encoding
inline std::string percent_encode(char input, std::predicate<char> auto in_encode_set, bool space_as_plus = false) {
    if (space_as_plus && input == ' ') {
        return std::string{'+'};
    }

    if (in_encode_set(input)) {
        return std::format("%{:02X}", input);
    }

    return std::string{input};
}

inline std::string percent_encode(
        std::string_view input, std::predicate<char> auto in_encode_set, bool space_as_plus = false) {
    std::stringstream out;

    for (char i : input) {
        out << percent_encode(i, in_encode_set, space_as_plus);
    }

    return std::move(out).str();
}

// https://url.spec.whatwg.org/#percent-decode
constexpr std::string percent_decode(std::string_view input) {
    std::string output;

    for (std::size_t i = 0; i < input.size(); i++) {
        if (input[i] != '%'
                || (input.size() <= i + 2 || !util::is_hex_digit(input[i + 1]) || !util::is_hex_digit(input[i + 2]))) {
            output += input[i];
        } else {
            std::string_view digits = input.substr(i + 1, 2);
            std::uint8_t num{};

            [[maybe_unused]] auto res = std::from_chars(digits.data(), digits.data() + digits.size(), num, 16);

            assert(res.ec != std::errc::invalid_argument && res.ec != std::errc::result_out_of_range);

            output += static_cast<char>(num);

            i += 2;
        }
    }

    return output;
}

// RFC3986 normalization; uppercase all percent-encoded triplets.
constexpr std::string percent_encoded_triplets_to_upper(std::string_view input) {
    std::string output;

    for (std::size_t i = 0; i < input.size(); i++) {
        if (input[i] == '%'
                && (input.size() > i + 2 && util::is_hex_digit(input[i + 1]) && util::is_hex_digit(input[i + 2]))) {
            output += input[i];
            output += util::uppercased(input[i + 1]);
            output += util::uppercased(input[i + 2]);

            i += 2;
        } else {
            output += input[i];
        }
    }

    return output;
}

// RFC3986 normalization; decode percent-encoded triplets that encode unreserved characters
constexpr std::string percent_decode_unreserved(std::string_view input) {
    std::string output;

    for (std::size_t i = 0; i < input.size(); i++) {
        if (input[i] != '%'
                || (input.size() <= i + 2 || !util::is_hex_digit(input[i + 1]) || !util::is_hex_digit(input[i + 2]))) {
            output += input[i];
        } else {
            std::string_view digits = input.substr(i + 1, 2);
            std::uint8_t num{};

            [[maybe_unused]] auto res = std::from_chars(digits.data(), digits.data() + digits.size(), num, 16);

            assert(res.ec != std::errc::invalid_argument && res.ec != std::errc::result_out_of_range);

            if (num > 127
                    || (!util::is_alpha(num) && !util::is_digit(num) && num != '-' && num != '.' && num != '_'
                            && num != '~')) {
                output += input[i];

                continue;
            }

            output += static_cast<char>(num);

            i += 2;
        }
    }

    return output;
}

} // namespace url

#endif
