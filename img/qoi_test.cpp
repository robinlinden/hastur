// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "img/qoi.h"

#include "etest/etest2.h"

#include <tl/expected.hpp>

#include <sstream>
#include <string>

using img::Qoi;
using img::QoiError;

using namespace std::literals;

int main() {
    etest::Suite s;

    s.add_test("abrupt eof before magic", [](etest::IActions &a) {
        a.expect_eq(Qoi::from(std::stringstream{"qoi"s}), tl::unexpected{QoiError::AbruptEof}); //
    });

    s.add_test("invalid magic", [](etest::IActions &a) {
        a.expect_eq(Qoi::from(std::stringstream{"qoib"s}), tl::unexpected{QoiError::InvalidMagic}); //
    });

    s.add_test("abrupt eof before width", [](etest::IActions &a) {
        a.expect_eq(Qoi::from(std::stringstream{"qoif\1\0\0"s}), tl::unexpected{QoiError::AbruptEof}); //
    });

    s.add_test("abrupt eof before height", [](etest::IActions &a) {
        a.expect_eq(Qoi::from(std::stringstream{"qoif\1\0\0\0\1\0\0"s}), tl::unexpected{QoiError::AbruptEof}); //
    });

    s.add_test("unreasonably large image", [](etest::IActions &a) {
        a.expect_eq(Qoi::from(std::stringstream{"qoif\1\0\0\0\1\0\0\0"s}), tl::unexpected{QoiError::ImageTooLarge}); //
    });

    s.add_test("channels error handling", [](etest::IActions &a) {
        a.expect_eq(Qoi::from(std::stringstream{"qoif\0\0\0\1\0\0\0\1"s}), tl::unexpected{QoiError::AbruptEof});
        a.expect_eq(Qoi::from(std::stringstream{"qoif\0\0\0\1\0\0\0\1\5"s}), tl::unexpected{QoiError::InvalidChannels});
    });

    s.add_test("colorspace error handling", [](etest::IActions &a) {
        a.expect_eq(Qoi::from(std::stringstream{"qoif\0\0\0\1\0\0\0\1\3"s}), tl::unexpected{QoiError::AbruptEof});
        a.expect_eq(
                Qoi::from(std::stringstream{"qoif\0\0\0\1\0\0\0\1\3\2"s}), tl::unexpected{QoiError::InvalidColorspace});
    });

    s.add_test("missing pixel data", [](etest::IActions &a) {
        a.expect_eq(Qoi::from(std::stringstream{"qoif\0\0\0\1\0\0\0\2\3\1"s}), tl::unexpected{QoiError::AbruptEof}); //
    });

    s.add_test("QOI_OP_RGB w/o pixel data", [](etest::IActions &a) {
        a.expect_eq(Qoi::from(std::stringstream{"qoif\0\0\0\1\0\0\0\2\3\1\xfe\1\2"s}),
                tl::unexpected{QoiError::AbruptEof}); //
    });

    s.add_test("QOI_OP_RGBA w/o pixel data", [](etest::IActions &a) {
        a.expect_eq(Qoi::from(std::stringstream{"qoif\0\0\0\1\0\0\0\2\3\1\xff\1\2"s}),
                tl::unexpected{QoiError::AbruptEof}); //
    });

    s.add_test("QOI_OP_RGBA", [](etest::IActions &a) {
        a.expect_eq(Qoi::from(std::stringstream{"qoif\0\0\0\1\0\0\0\1\3\1\xff\1\2\3\4\0\0\0\0\0\0\0\1"s}),
                img::Qoi{.width = 1, .height = 1, .bytes{1, 2, 3, 4}}); //
    });

    s.add_test("QOI_OP_INDEX w/o any pixel values seen", [](etest::IActions &a) {
        a.expect_eq(Qoi::from(std::stringstream{"qoif\0\0\0\1\0\0\0\1\3\1\0\0\0\0\0\0\0\0\1"s}),
                Qoi{.width = 1, .height = 1, .bytes{0, 0, 0, 0}}); //
    });

    s.add_test("QOI_OP_INDEX, write a pixel and read it back", [](etest::IActions &a) {
        // Carefully crafted pixel to have it end up in slot 0 in the seen pixels array.
        a.expect_eq(Qoi::from(std::stringstream{"qoif\0\0\0\2\0\0\0\1\3\1\xfe\1\x28\0\0\0\0\0\0\0\0\0\1"s}),
                Qoi{.width = 2, .height = 1, .bytes{1, 40, 0, 255, 1, 40, 0, 255}}); //
    });

    s.add_test("QOI_OP_RUN", [](etest::IActions &a) {
        a.expect_eq(Qoi::from(std::stringstream{"qoif\0\0\0\3\0\0\0\1\3\1\xc2\0\0\0\0\0\0\0\1"s}),
                Qoi{.width = 3, .height = 1, .bytes{0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255}});
    });

    s.add_test("QOI_OP_DIFF", [](etest::IActions &a) {
        // diff of {-2, -1, 1}, {1, 1, 1}
        a.expect_eq(Qoi::from(std::stringstream{"qoif\0\0\0\1\0\0\0\2\3\1\x47\x7f\0\0\0\0\0\0\0\1"s}),
                Qoi{.width = 1, .height = 2, .bytes{254, 255, 1, 255, 255, 0, 2, 255}});
    });

    s.add_test("QOI_OP_LUMA", [](etest::IActions &a) {
        // diff of {-24, -16, -9}, {25, 18, 22}
        a.expect_eq(Qoi::from(std::stringstream{"qoif\0\0\0\2\0\0\0\1\3\1\x90\x0f\xb2\xfc\0\0\0\0\0\0\0\1"s}),
                Qoi{.width = 2, .height = 1, .bytes{232, 240, 247, 255, 1, 2, 13, 255}});
    });

    s.add_test("QOI_OP_LUMA, missing extra byte", [](etest::IActions &a) {
        // diff of {-24, -16, -9}, {25, 18, 22}
        a.expect_eq(Qoi::from(std::stringstream{"qoif\0\0\0\2\0\0\0\1\3\1\x90"s}), //
                tl::unexpected{QoiError::AbruptEof});
    });

    s.add_test("0x0 image", [](etest::IActions &a) {
        a.expect_eq(Qoi::from(std::stringstream{"qoif\0\0\0\0\0\0\0\0\3\1\0\0\0\0\0\0\0\1"s}), Qoi{}); //
    });

    s.add_test("missing footer", [](etest::IActions &a) {
        a.expect_eq(Qoi::from(std::stringstream{"qoif\0\0\0\0\0\0\0\0\3\1"s}), //
                tl::unexpected{QoiError::AbruptEof});
    });

    s.add_test("invalid footer", [](etest::IActions &a) {
        a.expect_eq(Qoi::from(std::stringstream{"qoif\0\0\0\0\0\0\0\0\3\1\0\0\0\0\0\0\0\2"s}),
                tl::unexpected{QoiError::InvalidEndMarker});
    });

    s.add_test("it works", [](etest::IActions &a) {
        a.expect_eq(Qoi::from(std::stringstream{"qoif\0\0\0\1\0\0\0\2\3\1\xfe\1\2\3\xfe\6\5\4\0\0\0\0\0\0\0\1"s}),
                Qoi{.width = 1, .height = 2, .bytes{1, 2, 3, 255, 6, 5, 4, 255}});
    });

    return s.run();
}
