// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "img/gif.h"

#include "etest/etest.h"

#include <optional>
#include <sstream>
#include <string>

using etest::expect_eq;
using img::Gif;

using namespace std::literals;

int main() {
    etest::test("invalid signatures", [] {
        expect_eq(Gif::from(std::stringstream{"GIF87"s}), std::nullopt);
        expect_eq(Gif::from(std::stringstream{"GIF87b"s}), std::nullopt);
    });

    etest::test("version, width, and height", [] {
        Gif expected{.version = Gif::Version::Gif89a, .width = 3, .height = 5};
        expect_eq(Gif::from(std::stringstream{"GIF89a\3\0\5\0\0\0\0"s}), expected);
        expected = Gif{.version = Gif::Version::Gif87a, .width = 15000, .height = 1};
        expect_eq(Gif::from(std::stringstream{"GIF87a\x98\x3a\1\0\0\0\0"s}), expected);
    });

    etest::test("eof at height, width", [] {
        expect_eq(Gif::from(std::stringstream{"GIF87a"s}), std::nullopt);
        expect_eq(Gif::from(std::stringstream{"GIF89a\1\1\1"s}), std::nullopt);
    });

    etest::test("eof at screen descriptor packed fields", [] {
        expect_eq(Gif::from(std::stringstream{"GIF89a\1\1\1\1"s}), std::nullopt); //
    });

    etest::test("eof at screen descriptor background color index", [] {
        expect_eq(Gif::from(std::stringstream{"GIF89a\1\1\1\1\1"s}), std::nullopt); //
    });

    etest::test("eof at screen descriptor pixel aspect ratio", [] {
        expect_eq(Gif::from(std::stringstream{"GIF89a\1\1\1\1\1\1"s}), std::nullopt); //
    });

    etest::test("missing global color table", [] {
        expect_eq(Gif::from(std::stringstream{"GIF89a\1\0\1\0\x80\0\0"s}), std::nullopt); //
    });

    etest::test("global color table", [] {
        img::Gif expected{.version = img::Gif::Version::Gif89a, .width = 1, .height = 1};
        expect_eq(Gif::from(std::stringstream{"GIF89a\1\0\1\0\x80\0\0\1\2\3\1\2\3"s}), expected);
    });

    return etest::run_all_tests();
}
