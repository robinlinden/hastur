// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "util/unicode.h"

#include "etest/etest.h"

#include <string_view>

using namespace std::literals;

using etest::expect_eq;
using util::unicode_to_utf8;
using util::unicode_utf8_byte_count;

int main() {
    etest::test("unicode_utf8_byte_count", [] {
        expect_eq(unicode_utf8_byte_count(0), 1);
        expect_eq(unicode_utf8_byte_count(0x7f), 1);

        expect_eq(unicode_utf8_byte_count(0x80), 2);
        expect_eq(unicode_utf8_byte_count(0x7ff), 2);

        expect_eq(unicode_utf8_byte_count(0x800), 3);
        expect_eq(unicode_utf8_byte_count(0xffff), 3);

        expect_eq(unicode_utf8_byte_count(0x100000), 4);
        expect_eq(unicode_utf8_byte_count(0x10ffff), 4);

        // Invalid code points return 0.
        expect_eq(unicode_utf8_byte_count(0x110000), 0);
    });

    etest::test("unicode_to_utf8", [] {
        expect_eq(unicode_to_utf8(0x002f), "/"sv);

        expect_eq(unicode_to_utf8(0x00a3), "£"sv);
        expect_eq(unicode_to_utf8(0x07f9), "߹"sv);

        expect_eq(unicode_to_utf8(0x0939), "ह"sv);
        expect_eq(unicode_to_utf8(0x20ac), "€"sv);
        expect_eq(unicode_to_utf8(0xd55c), "한"sv);
        expect_eq(unicode_to_utf8(0xfffd), "�"sv);

        expect_eq(unicode_to_utf8(0x10348), "𐍈"sv);

        // Invalid code points return "".
        expect_eq(unicode_to_utf8(0x110000), ""sv);
    });

    return etest::run_all_tests();
}
