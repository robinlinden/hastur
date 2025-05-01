// SPDX-FileCopyrightText: 2023-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef WASM_LEB128_H_
#define WASM_LEB128_H_

#include <tl/expected.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <istream>

namespace wasm {
namespace detail {
// TODO(robinlinden): Switch to std::ceil once https://wg21.link/P0533R9 is implemented.
constexpr int ceil(float f) {
    int i = static_cast<int>(f);
    return f > i ? i + 1 : i;
}
} // namespace detail

enum class Leb128ParseError : std::uint8_t {
    Invalid,
    NonZeroExtraBits,
    UnexpectedEof,
};

// https://webassembly.github.io/spec/core/binary/values.html#integers
template<typename T>
requires std::integral<T>
struct Leb128 {};

// https://en.wikipedia.org/wiki/LEB128#Decode_unsigned_integer
template<std::unsigned_integral T>
struct Leb128<T> {
    static tl::expected<T, Leb128ParseError> decode_from(std::istream &&is) { return decode_from(is); }
    static tl::expected<T, Leb128ParseError> decode_from(std::istream &is) {
        T result{};
        std::uint8_t shift{};
        constexpr auto kMaxBytes = detail::ceil(sizeof(T) * 8 / 7.f);
        for (int i = 0; i < kMaxBytes; ++i) {
            std::uint8_t byte{};
            if (!is.read(reinterpret_cast<char *>(&byte), sizeof(byte))) {
                return tl::unexpected{Leb128ParseError::UnexpectedEof};
            }

            if (i == kMaxBytes - 1) {
                // This is the last byte we'll read. Check that any extra bits are all 0.
                constexpr auto kRemainingValueBits = sizeof(T) * 8 - (kMaxBytes - 1) * std::size_t{7};
                static_assert(kRemainingValueBits < 8);
                constexpr auto kExtraBitsMask = (0xff << kRemainingValueBits) & 0b0111'1111;
                auto extra_bits = byte & kExtraBitsMask;
                if (extra_bits != 0) {
                    return tl::unexpected{Leb128ParseError::NonZeroExtraBits};
                }
            }

            result |= static_cast<T>(byte & 0b0111'1111) << shift;
            if ((byte & 0b1000'0000) == 0) {
                return result;
            }

            shift += 7;
        }

        return tl::unexpected{Leb128ParseError::Invalid};
    }
};

// https://en.wikipedia.org/wiki/LEB128#Decode_signed_integer
template<std::signed_integral T>
struct Leb128<T> {
    static constexpr std::uint8_t kContinuationBit = 0b1000'0000;
    static constexpr std::uint8_t kNonContinuationBits = 0b0111'1111;
    static constexpr std::uint8_t kSignBit = 0b0100'0000;

    static tl::expected<T, Leb128ParseError> decode_from(std::istream &&is) { return decode_from(is); }
    static tl::expected<T, Leb128ParseError> decode_from(std::istream &is) {
        T result{};
        std::uint8_t shift{};
        std::uint8_t byte{};
        constexpr auto kMaxBytes = detail::ceil(sizeof(T) * 8 / 7.f);
        for (int i = 0; i < kMaxBytes; ++i) {
            if (!is.read(reinterpret_cast<char *>(&byte), sizeof(byte))) {
                return tl::unexpected{Leb128ParseError::UnexpectedEof};
            }

            if (i == kMaxBytes - 1) {
                // This is the last byte we'll read. Check that any extra bits are all 0.
                constexpr auto kRemainingValueBits = sizeof(T) * 8 - (kMaxBytes - 1) * std::size_t{7} - 1;
                static_assert(kRemainingValueBits < 8);
                constexpr auto kExtraBitsMask = (0xff << kRemainingValueBits) & kNonContinuationBits;
                auto extra_bits = byte & kExtraBitsMask;
                if (extra_bits != 0 && extra_bits != kExtraBitsMask) {
                    return tl::unexpected{Leb128ParseError::NonZeroExtraBits};
                }
            }

            result |= static_cast<T>(byte & kNonContinuationBits) << shift;
            shift += 7;
            if ((byte & kContinuationBit) == 0) {
                break;
            }
        }

        if ((byte & kContinuationBit) != 0) {
            return tl::unexpected{Leb128ParseError::Invalid};
        }

        if ((shift < sizeof(T) * 8) && ((byte & kSignBit) != 0)) {
            result |= ~T{0} << shift;
        }

        return result;
    }
};

} // namespace wasm

#endif
