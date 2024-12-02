// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "img/gif.h"

#include "etest/etest2.h"

#include <optional>
#include <sstream>
#include <string>

using img::Gif;

using namespace std::literals;

int main() {
    etest::Suite s;

    s.add_test("invalid signatures", [](etest::IActions &a) {
        a.expect_eq(Gif::from(std::stringstream{"GIF87"s}), std::nullopt);
        a.expect_eq(Gif::from(std::stringstream{"GIF87b"s}), std::nullopt);
    });

    s.add_test("version, width, and height", [](etest::IActions &a) {
        Gif expected{.version = Gif::Version::Gif89a, .width = 3, .height = 5};
        a.expect_eq(Gif::from(std::stringstream{"GIF89a\3\0\5\0\0\0\0"s}), expected);
        expected = Gif{.version = Gif::Version::Gif87a, .width = 15000, .height = 1};
        a.expect_eq(Gif::from(std::stringstream{"GIF87a\x98\x3a\1\0\0\0\0"s}), expected);
    });

    s.add_test("eof at height, width", [](etest::IActions &a) {
        a.expect_eq(Gif::from(std::stringstream{"GIF87a"s}), std::nullopt);
        a.expect_eq(Gif::from(std::stringstream{"GIF89a\1\1\1"s}), std::nullopt);
    });

    s.add_test("eof at screen descriptor packed fields", [](etest::IActions &a) {
        a.expect_eq(Gif::from(std::stringstream{"GIF89a\1\1\1\1"s}), std::nullopt); //
    });

    s.add_test("eof at screen descriptor background color index", [](etest::IActions &a) {
        a.expect_eq(Gif::from(std::stringstream{"GIF89a\1\1\1\1\1"s}), std::nullopt); //
    });

    s.add_test("eof at screen descriptor pixel aspect ratio", [](etest::IActions &a) {
        a.expect_eq(Gif::from(std::stringstream{"GIF89a\1\1\1\1\1\1"s}), std::nullopt); //
    });

    s.add_test("missing global color table", [](etest::IActions &a) {
        a.expect_eq(Gif::from(std::stringstream{"GIF89a\1\0\1\0\x80\0\0"s}), std::nullopt); //
    });

    s.add_test("global color table", [](etest::IActions &a) {
        img::Gif expected{.version = img::Gif::Version::Gif89a, .width = 1, .height = 1};
        a.expect_eq(Gif::from(std::stringstream{"GIF89a\1\0\1\0\x80\0\0\1\2\3\1\2\3"s}), expected);
    });

    return s.run();
}
