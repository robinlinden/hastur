// SPDX-FileCopyrightText: 2022-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "img/png.h"

#include "img/tiny_png.h"

#include "etest/etest2.h"

#include <array>
#include <cstddef>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

int main() {
    etest::Suite s;

    s.add_test("it works", [](etest::IActions &a) {
        auto expected_pixels = [] {
            std::array<unsigned char, 4> pixel_pattern{181, 208, 208, 0xff};
            std::vector<unsigned char> expected;
            static constexpr auto kPixelCount = std::size_t{256} * 256 * 4;
            expected.resize(kPixelCount);

            for (std::size_t i = 0; i < expected.size(); ++i) {
                expected[i] = pixel_pattern[i % pixel_pattern.size()];
            }

            return expected;
        }();

        auto png = img::Png::from(std::stringstream(std::string{kTinyPng})).value();
        a.expect_eq(png, img::Png{.width = 256, .height = 256, .bytes = std::move(expected_pixels)});
    });

    s.add_test("invalid signatures are rejected", [](etest::IActions &a) {
        auto invalid_signature_bytes = std::string{kTinyPng};
        invalid_signature_bytes[7] = 'b';
        a.expect_eq(img::Png::from(std::stringstream(std::move(invalid_signature_bytes))), std::nullopt);
    });

    s.add_test("it handles truncated files", [](etest::IActions &a) {
        auto truncated_bytes = std::string{kTinyPng.substr(0, 30)};
        a.expect_eq(img::Png::from(std::stringstream(std::move(truncated_bytes))), std::nullopt);
    });

    return s.run();
}
