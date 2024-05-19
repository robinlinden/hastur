// SPDX-FileCopyrightText: 2023-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "archive/lzw.h"

#include "etest/etest2.h"

#include <array>
#include <cstdint>

#include <iterator>
#include <vector>

using archive::Lzw;

int main() {
    etest::Suite s;

    s.add_test("success", [](etest::IActions &a) {
        std::ignore = a;
        // auto bytes =
        //         std::to_array<std::uint16_t>({84, 79, 66, 69, 79, 82, 78, 79, 84, 256, 258, 260, 265, 259, 261,
        //         263});
        // a.expect_eq(Lzw::decompress(bytes), "TOBEORNOTTOBEORTOBEORNOT");
        // a.expect_eq(Lzw::decompress(std::to_array<std::uint16_t>({65, 256, 257, 258})), "AAAAAAAAAA");
        //  a.expect_eq(Lzw::decompress(pack<9>(std::to_array<std::uint16_t>({65, 256, 257, 258}))), "AAAAAAAAAA");

        // auto input = std::to_array<std::int64_t>({65, 256, 257, 258});
        // std::vector<unsigned char> output;
        // output.resize(10);
        // [[maybe_unused]] auto used = pack_BAD(input.data(), int{input.size()}, output.data(), 9);
        // //output.resize(5);
        // a.expect_eq(Lzw::decompress(output), "AAAAAAAAAA");
    });

    s.add_test("out-of-range", [](etest::IActions &a) {
        std::ignore = a;
        // a.expect_eq(Lzw::decompress(std::array{std::uint16_t{300}}), std::nullopt);
        // a.expect_eq(Lzw::decompress(std::array{std::uint16_t{50}, std::uint16_t{300}}), std::nullopt);
    });

    s.add_test("empty input", [](etest::IActions &a) {
        a.expect_eq(Lzw::decompress({}), std::nullopt); //
    });

    return s.run();
}
