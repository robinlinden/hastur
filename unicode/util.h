// SPDX-FileCopyrightText: 2022-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef UNICODE_UTIL_H_
#define UNICODE_UTIL_H_

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace unicode {

constexpr bool is_ascii(std::uint32_t code_point) {
    return code_point <= 0x7f;
}

constexpr std::optional<std::uint8_t> utf8_byte_count(std::uint32_t code_point) {
    if (is_ascii(code_point)) {
        return std::uint8_t{1};
    }

    if (code_point <= 0x7ff) {
        return std::uint8_t{2};
    }

    if (code_point <= 0xffff) {
        return std::uint8_t{3};
    }

    if (code_point <= 0x10ffff) {
        return std::uint8_t{4};
    }

    return std::nullopt;
}

constexpr std::string to_utf8(std::uint32_t code_point) {
    switch (utf8_byte_count(code_point).value_or(0)) {
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

constexpr bool is_high_surrogate(std::uint32_t code_point) {
    return code_point >= 0xD800 && code_point <= 0xDBFF;
}

constexpr bool is_low_surrogate(std::uint32_t code_point) {
    return code_point >= 0xDC00 && code_point <= 0xDFFF;
}

// https://infra.spec.whatwg.org/#surrogate
constexpr bool is_surrogate(std::uint32_t code_point) {
    return is_high_surrogate(code_point) || is_low_surrogate(code_point);
}

constexpr std::optional<std::uint32_t> utf16_surrogate_pair_to_code_point(std::uint16_t high, std::uint16_t low) {
    if (!is_high_surrogate(high) || !is_low_surrogate(low)) {
        return std::nullopt;
    }

    return 0x10000 + ((high & 0x3FF) << 10) + (low & 0x3FF);
}

constexpr std::optional<std::string> utf16_to_utf8(std::uint16_t code_unit) {
    if (is_surrogate(code_unit)) {
        return std::nullopt;
    }

    return to_utf8(code_unit);
}

// https://infra.spec.whatwg.org/#noncharacter
constexpr bool is_noncharacter(std::uint32_t code_point) {
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

// TODO(robinlinden): Only allow use w/ valid UTF-8.
class CodePointView {
    class CodePointIterator;

public:
    constexpr explicit CodePointView(std::string_view utf8_data) : view_{std::move(utf8_data)} {}

    constexpr CodePointIterator begin() const { return CodePointIterator{view_.begin()}; }
    constexpr CodePointIterator end() const { return CodePointIterator{view_.end()}; }

private:
    std::string_view view_;

    class CodePointIterator {
    public:
        constexpr explicit CodePointIterator(std::string_view::const_iterator it) : it_{std::move(it)} {}

        constexpr CodePointIterator &operator++() {
            it_ += current_code_point_length();
            return *this;
        }

        constexpr CodePointIterator operator++(int) {
            auto copy = *this;
            ++*this;
            return copy;
        }

        constexpr std::uint32_t operator*() const {
            return utf8_to_utf32(std::string_view{it_, it_ + current_code_point_length()});
        }

        [[nodiscard]] constexpr bool operator==(CodePointIterator const &) const = default;

    private:
        static constexpr auto kTwoByteMask = 0b1100'0000;
        static constexpr auto kThreeByteMask = 0b1110'0000;
        static constexpr auto kFourByteMask = 0b1111'0000;

        std::string_view::const_iterator it_;

        constexpr int current_code_point_length() const {
            auto const current = *it_;

            if ((current & kFourByteMask) == kFourByteMask) {
                return 4;
            }

            if ((current & kThreeByteMask) == kThreeByteMask) {
                return 3;
            }

            if ((current & kTwoByteMask) == kTwoByteMask) {
                return 2;
            }

            return 1;
        }
    };
};

} // namespace unicode

#endif
