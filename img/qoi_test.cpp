// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "img/qoi.h"

#include "etest/etest.h"

#include <tl/expected.hpp>

#include <sstream>
#include <string>

using etest::expect_eq;
using img::Qoi;
using img::QoiError;

using namespace std::literals;

int main() {
    etest::test("abrupt eof before magic", [] {
        expect_eq(Qoi::from(std::stringstream{"qoi"s}), tl::unexpected{QoiError::AbruptEof}); //
    });

    etest::test("invalid magic", [] {
        expect_eq(Qoi::from(std::stringstream{"qoib"s}), tl::unexpected{QoiError::InvalidMagic}); //
    });

    etest::test("abrupt eof before width", [] {
        expect_eq(Qoi::from(std::stringstream{"qoif\1\0\0"s}), tl::unexpected{QoiError::AbruptEof}); //
    });

    etest::test("abrupt eof before height", [] {
        expect_eq(Qoi::from(std::stringstream{"qoif\1\0\0\0\1\0\0"s}), tl::unexpected{QoiError::AbruptEof}); //
    });

    etest::test("channels error handling", [] {
        expect_eq(Qoi::from(std::stringstream{"qoif\1\0\0\0\1\0\0\0"s}), tl::unexpected{QoiError::AbruptEof});
        expect_eq(Qoi::from(std::stringstream{"qoif\1\0\0\0\1\0\0\0\5"s}), tl::unexpected{QoiError::InvalidChannels});
    });

    etest::test("colorspace error handling", [] {
        expect_eq(Qoi::from(std::stringstream{"qoif\1\0\0\0\1\0\0\0\3"s}), tl::unexpected{QoiError::AbruptEof});
        expect_eq(
                Qoi::from(std::stringstream{"qoif\1\0\0\0\1\0\0\0\3\2"s}), tl::unexpected{QoiError::InvalidColorspace});
    });

    etest::test("missing pixel data", [] {
        expect_eq(Qoi::from(std::stringstream{"qoif\0\0\0\1\0\0\0\2\3\1"s}), tl::unexpected{QoiError::AbruptEof}); //
    });

    etest::test("unsupported chunk", [] {
        expect_eq(Qoi::from(std::stringstream{"qoif\0\0\0\1\0\0\0\2\3\1\xff"s}),
                tl::unexpected{QoiError::UnhandledChunk}); //
    });

    etest::test("QOI_OP_RGB w/o pixel data", [] {
        expect_eq(Qoi::from(std::stringstream{"qoif\0\0\0\1\0\0\0\2\3\1\xfe\1\2"s}),
                tl::unexpected{QoiError::AbruptEof}); //
    });

    etest::test("QOI_OP_INDEX w/o any pixel values seen", [] {
        expect_eq(Qoi::from(std::stringstream{"qoif\0\0\0\1\0\0\0\1\3\1\0"s}),
                Qoi{.width = 1, .height = 1, .bytes{0, 0, 0, 0}}); //
    });

    etest::test("QOI_OP_INDEX, write a pixel and read it back", [] {
        // Carefully crafted pixel to have it end up in slot 0 in the seen pixels array.
        expect_eq(Qoi::from(std::stringstream{"qoif\0\0\0\2\0\0\0\1\3\1\xfe\1\x28\0\0"s}),
                Qoi{.width = 2, .height = 1, .bytes{1, 40, 0, 255, 1, 40, 0, 255}}); //
    });

    etest::test("0x0 image", [] {
        expect_eq(Qoi::from(std::stringstream{"qoif\0\0\0\0\0\0\0\0\3\1\xfe\1\2\3\xfe\6\5\4"s}), Qoi{}); //
    });

    etest::test("it works", [] {
        expect_eq(Qoi::from(std::stringstream{"qoif\0\0\0\1\0\0\0\2\3\1\xfe\1\2\3\xfe\6\5\4"s}),
                Qoi{.width = 1, .height = 2, .bytes{1, 2, 3, 255, 6, 5, 4, 255}});
    });

    return etest::run_all_tests();
}
