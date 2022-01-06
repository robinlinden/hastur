// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef UTIL_UNICODE_H_
#define UTIL_UNICODE_H_

#include <cstdint>
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

inline std::string unicode_to_utf8(std::uint32_t code_point) {
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

} // namespace util

#endif
