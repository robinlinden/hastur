// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef WASM_LEB128_H_
#define WASM_LEB128_H_

#include <tl/expected.hpp>

#include <cassert>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <istream>

namespace wasm {

enum class Leb128ParseError {
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
        auto const max_bytes = static_cast<int>(std::ceil(sizeof(T) * 8 / 7.f));
        for (int i = 0; i < max_bytes; ++i) {
            std::uint8_t byte{};
            if (!is.read(reinterpret_cast<char *>(&byte), sizeof(byte))) {
                return tl::unexpected{Leb128ParseError::UnexpectedEof};
            }

            if (i == max_bytes - 1) {
                // This is the last byte we'll read. Check that any extra bits are all 0.
                auto remaining_value_bits = sizeof(T) * 8 - (max_bytes - 1) * std::size_t{7};
                assert(remaining_value_bits < 8);
                auto extra_bits_mask = (0xff << remaining_value_bits) & 0b0111'1111;
                auto extra_bits = byte & extra_bits_mask;
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
        auto const max_bytes = static_cast<int>(std::ceil(sizeof(T) * 8 / 7.f));
        for (int i = 0; i < max_bytes; ++i) {
            if (!is.read(reinterpret_cast<char *>(&byte), sizeof(byte))) {
                return tl::unexpected{Leb128ParseError::UnexpectedEof};
            }

            if (i == max_bytes - 1) {
                // This is the last byte we'll read. Check that any extra bits are all 0.
                auto remaining_value_bits = sizeof(T) * 8 - (max_bytes - 1) * std::size_t{7} - 1;
                assert(remaining_value_bits < 8);
                auto extra_bits_mask = (0xff << remaining_value_bits) & kNonContinuationBits;
                auto extra_bits = byte & extra_bits_mask;
                if (extra_bits != 0 && extra_bits != extra_bits_mask) {
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
