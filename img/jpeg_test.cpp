// SPDX-FileCopyrightText: 2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "img/jpeg.h"

#include "etest/etest2.h"

#include <optional>
#include <sstream>
#include <string>

using namespace std::literals;

int main() {
    etest::Suite s;

    s.add_test("soi marker eof", [](etest::IActions &a) {
        auto jpeg = img::Jpeg::thumbnail_from(std::istringstream{"\xAB"});
        a.expect_eq(jpeg, std::nullopt);
    });

    s.add_test("soi marker invalid", [](etest::IActions &a) {
        auto jpeg = img::Jpeg::thumbnail_from(std::istringstream{"\xAB\xCD"});
        a.expect_eq(jpeg, std::nullopt);
    });

    s.add_test("app0jfif marker eof", [](etest::IActions &a) {
        auto jpeg = img::Jpeg::thumbnail_from(std::istringstream{"\xFF\xD8"});
        a.expect_eq(jpeg, std::nullopt);
    });

    s.add_test("app0jfif marker invalid", [](etest::IActions &a) {
        auto jpeg = img::Jpeg::thumbnail_from(std::istringstream{"\xFF\xD8\xAB\xCD"});
        a.expect_eq(jpeg, std::nullopt);
    });

    s.add_test("app0jfif::length eof", [](etest::IActions &a) {
        auto jpeg = img::Jpeg::thumbnail_from(std::istringstream{"\xFF\xD8\xFF\xE0"});
        a.expect_eq(jpeg, std::nullopt);
    });

    s.add_test("app0jfif::identifier eof", [](etest::IActions &a) {
        auto jpeg = img::Jpeg::thumbnail_from(std::istringstream{"\xFF\xD8\xFF\xE0\x00\x10"s});
        a.expect_eq(jpeg, std::nullopt);
    });

    s.add_test("app0jfif::identifier invalid", [](etest::IActions &a) {
        auto jpeg = img::Jpeg::thumbnail_from(std::istringstream{"\xFF\xD8\xFF\xE0\x00\x10JFIFA"s});
        a.expect_eq(jpeg, std::nullopt);
    });

    s.add_test("app0jfif::version eof", [](etest::IActions &a) {
        auto jpeg = img::Jpeg::thumbnail_from(std::istringstream{"\xFF\xD8\xFF\xE0\x00\x10JFIF\0"s});
        a.expect_eq(jpeg, std::nullopt);
    });

    s.add_test("app0jfif::version unsupported", [](etest::IActions &a) {
        auto jpeg = img::Jpeg::thumbnail_from(std::istringstream{"\xFF\xD8\xFF\xE0\x00\x10JFIF\0\1\1\1\1"s});
        a.expect_eq(jpeg, std::nullopt);
    });

    s.add_test("app0jfif::units eof", [](etest::IActions &a) {
        auto jpeg = img::Jpeg::thumbnail_from(std::istringstream{"\xFF\xD8\xFF\xE0\x00\x10JFIF\0\x01\x02"s});
        a.expect_eq(jpeg, std::nullopt);
    });

    s.add_test("app0jfif::units invalid", [](etest::IActions &a) {
        auto jpeg = img::Jpeg::thumbnail_from(std::istringstream{"\xFF\xD8\xFF\xE0\x00\x10JFIF\0\x01\x02\x03"s});
        a.expect_eq(jpeg, std::nullopt);
    });

    s.add_test("app0jfif::x_density eof", [](etest::IActions &a) {
        auto jpeg = img::Jpeg::thumbnail_from(std::istringstream{"\xFF\xD8\xFF\xE0\x00\x10JFIF\0\x01\x02\x00"s});
        a.expect_eq(jpeg, std::nullopt);
    });

    s.add_test("app0jfif::x_density invalid", [](etest::IActions &a) {
        auto jpeg =
                img::Jpeg::thumbnail_from(std::istringstream{"\xFF\xD8\xFF\xE0\x00\x10JFIF\0\x01\x02\x00\x00\x00"s});
        a.expect_eq(jpeg, std::nullopt);
    });

    s.add_test("app0jfif::y_density eof", [](etest::IActions &a) {
        auto jpeg =
                img::Jpeg::thumbnail_from(std::istringstream{"\xFF\xD8\xFF\xE0\x00\x10JFIF\0\x01\x02\x00\x00\x10"s});
        a.expect_eq(jpeg, std::nullopt);
    });

    s.add_test("app0jfif::y_density invalid", [](etest::IActions &a) {
        auto jpeg = img::Jpeg::thumbnail_from(
                std::istringstream{"\xFF\xD8\xFF\xE0\x00\x10JFIF\0\x01\x02\x00\x00\x10\x00\x00"s});
        a.expect_eq(jpeg, std::nullopt);
    });

    s.add_test("app0jfif::x_thumbnail eof", [](etest::IActions &a) {
        auto jpeg = img::Jpeg::thumbnail_from(
                std::istringstream{"\xFF\xD8\xFF\xE0\x00\x10JFIF\0\x01\x02\x00\x00\x10\x00\x10"s});
        a.expect_eq(jpeg, std::nullopt);
    });

    s.add_test("app0jfif::y_thumbnail eof", [](etest::IActions &a) {
        auto jpeg = img::Jpeg::thumbnail_from(
                std::istringstream{"\xFF\xD8\xFF\xE0\x00\x10JFIF\0\x01\x02\x00\x00\x10\x00\x10\x00"s});
        a.expect_eq(jpeg, std::nullopt);
    });

    s.add_test("app0jfif::thumbnail_rgb eof", [](etest::IActions &a) {
        auto jpeg = img::Jpeg::thumbnail_from(
                std::istringstream{"\xFF\xD8\xFF\xE0\x00\x10JFIF\0\x01\x02\x00\x00\x10\x00\x10\x01\x01"s});
        a.expect_eq(jpeg, std::nullopt);
    });

    s.add_test("app0jfif no thumbnail", [](etest::IActions &a) {
        auto jpeg = img::Jpeg::thumbnail_from(
                std::istringstream{"\xFF\xD8\xFF\xE0\x00\x10JFIF\0\x01\x02\x00\x00\x10\x00\x10\x00\x00"s});
        a.expect_eq(jpeg, std::nullopt);
    });

    s.add_test("app0jfif thumbnail, aspect ratio", [](etest::IActions &a) {
        auto jpeg = img::Jpeg::thumbnail_from(
                std::istringstream{"\xFF\xD8\xFF\xE0\x00\x10JFIF\0\x01\x02\x00\x00\x10\x00\x10\x01\x01\xFF\x11\x22"s});
        a.expect_eq(jpeg, img::Jpeg{.width = 1, .height = 1, .bytes = {0xFF, 0x11, 0x22, 0xFF}});
    });

    s.add_test("app0jfif thumbnail, dots per inch", [](etest::IActions &a) {
        auto jpeg = img::Jpeg::thumbnail_from(
                std::istringstream{"\xFF\xD8\xFF\xE0\x00\x10JFIF\0\x01\x02\x01\x00\x10\x00\x10\x01\x01\xFF\x11\x22"s});
        a.expect_eq(jpeg, img::Jpeg{.width = 1, .height = 1, .bytes = {0xFF, 0x11, 0x22, 0xFF}});
    });

    s.add_test("app0jfif thumbnail, dots per cm", [](etest::IActions &a) {
        auto jpeg = img::Jpeg::thumbnail_from(
                std::istringstream{"\xFF\xD8\xFF\xE0\x00\x10JFIF\0\x01\x02\x02\x00\x10\x00\x10\x01\x01\xFF\x11\x22"s});
        a.expect_eq(jpeg, img::Jpeg{.width = 1, .height = 1, .bytes = {0xFF, 0x11, 0x22, 0xFF}});
    });

    return s.run();
}
