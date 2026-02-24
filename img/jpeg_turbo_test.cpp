// SPDX-FileCopyrightText: 2025-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "img/jpeg_turbo.h"

#include "img/tiny_jpg.h"

#include "etest/etest2.h"

#include <cstddef>
#include <span>
#include <sstream>
#include <tuple>

namespace {
// NOLINTNEXTLINE(bugprone-throwing-static-initialization): Why would this throw?
std::span<std::byte const> const jpg_bytes(std::as_bytes(std::span{kTinyJpg}));
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
        std::stringstream ss{std::string{kTinyJpg}};
        auto const image = img::JpegTurbo::from(ss).value();
        a.expect_eq(image, img::JpegTurbo::from(jpg_bytes).value());
    });

    return s.run();
}
