// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef WASM_LEB128_H_
#define WASM_LEB128_H_

#include <cmath>
#include <cstdint>
#include <istream>
#include <optional>

namespace wasm {

// https://en.wikipedia.org/wiki/LEB128#Decode_unsigned_integer
// https://webassembly.github.io/spec/core/bikeshed/#binary-int
struct Uleb128 {
    static std::optional<std::uint32_t> decode_from(std::istream &&is) { return decode_from(is); }
    static std::optional<std::uint32_t> decode_from(std::istream &is) {
        std::uint32_t result{};
        std::uint8_t shift{};
        for (int i = 0; i < std::ceil(sizeof(std::uint32_t) * 8 / 7.f); ++i) {
            std::uint8_t byte{};
            if (!is.read(reinterpret_cast<char *>(&byte), sizeof(byte))) {
                return std::nullopt;
            }

            result |= (byte & 0b0111'1111) << shift;
            if (!(byte & 0b1000'0000)) {
                return result;
            }

            shift += 7;
        }

        return std::nullopt;
    }
};

} // namespace wasm

#endif
