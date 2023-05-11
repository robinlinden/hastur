// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef UTIL_CRC32_H_
#define UTIL_CRC32_H_

#include <array>
#include <cstdint>
#include <span>

namespace util {

// https://www.w3.org/TR/2022/WD-png-3-20221025/#5CRC-algorithm
template<typename T, std::size_t U = std::dynamic_extent>
requires(sizeof(T) == 1)
constexpr std::uint32_t crc32(std::span<T const, U> data) {
    constexpr auto kCrcTable = [] {
        std::array<std::uint32_t, 256> t{};
        constexpr std::uint32_t kPolynomial = 0xEDB8'8320;

        for (std::uint32_t i = 0; i < t.size(); ++i) {
            std::uint32_t val = i;

            for (std::uint32_t j = 0; j < 8; ++j) {
                val = val & 1 ? kPolynomial ^ (val >> 1) : val >> 1;
            }

            t[i] = val;
        }

        return t;
    }();

    std::uint32_t crc{0xFFFF'FFFF};
    for (auto byte : data) {
        crc = kCrcTable[(crc ^ byte) & 0xFF] ^ crc >> 8;
    }
    return crc ^ 0xFFFF'FFFF;
}

} // namespace util

#endif
