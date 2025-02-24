// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "img/jpeg_turbo.h"

#include "etest/etest2.h"

#include <cstddef>
#include <span>
#include <sstream>
#include <tuple>

namespace {
#include "img/tiny_jpg.h"
// NOLINTNEXTLINE(cert-err58-cpp): Why would this throw?
std::span<std::byte const> const jpg_bytes(reinterpret_cast<std::byte const *>(img_tiny_jpg), img_tiny_jpg_len);
} // namespace

int main() {
    etest::Suite s;

    s.add_test("it can run", [](etest::IActions &) {
        std::ignore = img::JpegTurbo::from({}); //
    });

    s.add_test("valid image", [](etest::IActions &a) {
        auto const image = img::JpegTurbo::from(jpg_bytes).value();
        a.expect_eq(image.height, 1u);
        a.expect_eq(image.width, 1u);
    });

    // The same bytes should make the same image, span/ostream shouldn't matter.
    s.add_test("JpegTurbo::from(ostream &)", [](etest::IActions &a) {
        std::stringstream ss{std::string{reinterpret_cast<char const *>(img_tiny_jpg), img_tiny_jpg_len}};
        auto const image = img::JpegTurbo::from(ss).value();
        a.expect_eq(image, img::JpegTurbo::from(jpg_bytes).value());
    });

    return s.run();
}
