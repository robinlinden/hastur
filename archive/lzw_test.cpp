// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "archive/lzw.h"

#include "etest/etest.h"

#include <array>
#include <cstdint>

#include <iterator>
#include <vector>

using archive::Lzw;
using etest::expect_eq;

int main() {
    etest::test("success", [] {
        // auto bytes =
        //         std::to_array<std::uint16_t>({84, 79, 66, 69, 79, 82, 78, 79, 84, 256, 258, 260, 265, 259, 261,
        //         263});
        // expect_eq(Lzw::decompress(bytes), "TOBEORNOTTOBEORTOBEORNOT");
        // expect_eq(Lzw::decompress(std::to_array<std::uint16_t>({65, 256, 257, 258})), "AAAAAAAAAA");
        //  expect_eq(Lzw::decompress(pack<9>(std::to_array<std::uint16_t>({65, 256, 257, 258}))), "AAAAAAAAAA");

        // auto input = std::to_array<std::int64_t>({65, 256, 257, 258});
        // std::vector<unsigned char> output;
        // output.resize(10);
        // [[maybe_unused]] auto used = pack_BAD(input.data(), int{input.size()}, output.data(), 9);
        // //output.resize(5);
        // expect_eq(Lzw::decompress(output), "AAAAAAAAAA");
    });

    etest::test("out-of-range", [] {
        // expect_eq(Lzw::decompress(std::array{std::uint16_t{300}}), std::nullopt);
        // expect_eq(Lzw::decompress(std::array{std::uint16_t{50}, std::uint16_t{300}}), std::nullopt);
    });

    etest::test("empty input", [] {
        expect_eq(Lzw::decompress({}), std::nullopt); //
    });

    return etest::run_all_tests();
}
