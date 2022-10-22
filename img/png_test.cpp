// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "img/png.h"

#include "etest/etest.h"

#include <array>
#include <cstddef>
#include <sstream>
#include <utility>
#include <vector>

using etest::expect_eq;

namespace {
#include "img/tiny_png.h"
std::string const png_bytes(reinterpret_cast<char const *>(img_tiny_png), img_tiny_png_len);
} // namespace

int main() {
    etest::test("it works", [] {
        auto expected_pixels = [] {
            std::array<unsigned char, 3> pixel_pattern{181, 208, 208};
            std::vector<unsigned char> expected;
            expected.resize(256 * 256 * 3);

            for (std::size_t i = 0; i < expected.size(); ++i) {
                expected[i] = pixel_pattern[i % pixel_pattern.size()];
            }

            return expected;
        }();

        auto png = img::Png::from(std::stringstream(png_bytes)).value();
        expect_eq(png, img::Png{.width = 256, .height = 256, .bytes = std::move(expected_pixels)});
    });

    etest::test("invalid signatures are rejected", [] {
        auto invalid_signature_bytes = png_bytes;
        invalid_signature_bytes[7] = 'b';
        expect_eq(img::Png::from(std::stringstream(std::move(invalid_signature_bytes))), std::nullopt);
    });

    etest::test("it handles truncated files", [] {
        auto truncated_bytes = png_bytes.substr(0, 30);
        expect_eq(img::Png::from(std::stringstream(std::move(truncated_bytes))), std::nullopt);
    });

    return etest::run_all_tests();
}
