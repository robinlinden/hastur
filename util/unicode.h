// SPDX-FileCopyrightText: 2022-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef UTIL_UNICODE_H_
#define UTIL_UNICODE_H_

#include <cstdint>
#include <optional>
#include <string>

namespace util {

constexpr bool unicode_is_ascii(std::uint32_t code_point) {
    return code_point <= 0x7f;
}

constexpr int unicode_utf8_byte_count(std::uint32_t code_point) {
    if (unicode_is_ascii(code_point)) {
        return 1;
    }

    if (code_point <= 0x7ff) {
        return 2;
    }

    if (code_point <= 0xffff) {
        return 3;
    }

    if (code_point <= 0x10ffff) {
        return 4;
    }

    return 0;
}

constexpr std::string unicode_to_utf8(std::uint32_t code_point) {
    switch (unicode_utf8_byte_count(code_point)) {
        case 1:
            return {static_cast<char>(code_point & 0x7F)};
        case 2:
            return {
                    static_cast<char>((code_point >> 6 & 0b0001'1111) | 0b1100'0000),
                    static_cast<char>((code_point & 0b0011'1111) | 0b1000'0000),
            };
        case 3:
            return {
                    static_cast<char>((code_point >> 12 & 0b0000'1111) | 0b1110'0000),
                    static_cast<char>((code_point >> 6 & 0b0011'1111) | 0b1000'0000),
                    static_cast<char>((code_point & 0b0011'1111) | 0b1000'0000),
            };
        case 4:
            return {
                    static_cast<char>((code_point >> 18 & 0b0000'0111) | 0b1111'0000),
                    static_cast<char>((code_point >> 12 & 0b0011'1111) | 0b1000'0000),
                    static_cast<char>((code_point >> 6 & 0b0011'1111) | 0b1000'0000),
                    static_cast<char>((code_point & 0b0011'1111) | 0b1000'0000),
            };
        default:
            return std::string{};
    }
}

// https://infra.spec.whatwg.org/#surrogate
constexpr bool is_unicode_surrogate(std::uint32_t code_point) {
    return code_point >= 0xD800 && code_point <= 0xDFFF;
}

// https://infra.spec.whatwg.org/#noncharacter
constexpr bool is_unicode_noncharacter(std::uint32_t code_point) {
    if (code_point >= 0xFDD0 && code_point <= 0xFDEF) {
        return true;
    }

    switch (code_point) {
        case 0xFFFE:
        case 0xFFFF:
        case 0x1FFFE:
        case 0x1FFFF:
        case 0x2FFFE:
        case 0x2FFFF:
        case 0x3FFFE:
        case 0x3FFFF:
        case 0x4FFFE:
        case 0x4FFFF:
        case 0x5FFFE:
        case 0x5FFFF:
        case 0x6FFFE:
        case 0x6FFFF:
        case 0x7FFFE:
        case 0x7FFFF:
        case 0x8FFFE:
        case 0x8FFFF:
        case 0x9FFFE:
        case 0x9FFFF:
        case 0xAFFFE:
        case 0xAFFFF:
        case 0xBFFFE:
        case 0xBFFFF:
        case 0xCFFFE:
        case 0xCFFFF:
        case 0xDFFFE:
        case 0xDFFFF:
        case 0xEFFFE:
        case 0xEFFFF:
        case 0xFFFFE:
        case 0xFFFFF:
        case 0x10FFFE:
        case 0x10FFFF:
            return true;
        default:
            return false;
    }
}

// Takes a UTF-8 encoded codepoint, and returns the codepoint value.
//
// Note: This routine assumes that the input is a valid UTF-8 string. Strings that are too short return 0.
constexpr std::uint32_t utf8_to_utf32(std::string_view input) {
    std::uint32_t codepoint = 0;

    if (!input.empty() && (input[0] & 0b10000000) == 0b00000000) {
        codepoint = static_cast<unsigned char>(input[0]);
    } else if (input.size() > 1 && (input[0] & 0b11100000) == 0b11000000) {
        codepoint = ((input[0] & 0b00011111) << 6) | (input[1] & 0b00111111);
    } else if (input.size() > 2 && (input[0] & 0b11110000) == 0b11100000) {
        codepoint = ((input[0] & 0b00001111) << 12) | ((input[1] & 0b00111111) << 6) | (input[2] & 0b00111111);
    } else if (input.size() > 3 && (input[0] & 0b11111000) == 0b11110000) {
        codepoint = ((input[0] & 0b00000111) << 18) | ((input[1] & 0b00111111) << 12) | ((input[2] & 0b00111111) << 6)
                | (input[3] & 0b00111111);
    }

    return codepoint;
}

// Calculates codepoint length of a UTF-8 string.
//
// Note: This routine assumes that the string is valid UTF-8, otherwise we need
// to check if the bytes following the first byte of the codepoint are correct
// instead of just advancing the index.
//
// For incorrectly-encoded strings which do not have enough data to match the
// size suggested by the initial code unit, this function returns std::nullopt
constexpr std::optional<std::size_t> utf8_length(std::string_view input) {
    std::size_t len = 0;

    for (std::size_t i = 0; i < input.size(); i++) {
        if ((input[i] & 0b10000000) == 0b00000000) {
            len++;
        } else if ((input[i] & 0b11100000) == 0b11000000) {
            i++;

            if (input.size() <= i) {
                return std::nullopt;
            }

            len++;
        } else if ((input[i] & 0b11110000) == 0b11100000) {
            i += 2;

            if (input.size() <= i) {
                return std::nullopt;
            }

            len++;
        } else if ((input[i] & 0b11111000) == 0b11110000) {
            i += 3;

            if (input.size() <= i) {
                return std::nullopt;
            }

            len++;
        }
    }

    return len;
}

} // namespace util

#endif
