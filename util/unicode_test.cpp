// SPDX-FileCopyrightText: 2022-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "util/unicode.h"

#include "etest/etest.h"

#include <string_view>

using namespace std::literals;
using namespace util;

using etest::expect;
using etest::expect_eq;

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

    etest::test("is_unicode_surrogate", [] {
        expect(!is_unicode_surrogate(0xD799));
        expect(is_unicode_surrogate(0xD800)); // First leading surrogate.
        expect(is_unicode_surrogate(0xDBFF)); // Last leading surrogate.
        expect(is_unicode_surrogate(0xDC00)); // First trailing surrogate.
        expect(is_unicode_surrogate(0xDFFF)); // Last trailing surrogate.
        expect(!is_unicode_surrogate(0xE000));
    });

    etest::test("is_unicode_noncharacter", [] {
        expect(!is_unicode_noncharacter(0xFDD0 - 1));

        for (int i = 0xFDD0; i <= 0xFDEF; ++i) {
            expect(is_unicode_noncharacter(i));
        }

        expect(!is_unicode_noncharacter(0xFDEF + 1));
        expect(!is_unicode_noncharacter(0xFFFE - 1));

        // Every 0x10000 pair of values ending in FFFE and FFFF are noncharacters.
        for (int i = 0xFFFE; i <= 0x10FFFE; i += 0x10000) {
            expect(!is_unicode_noncharacter(i - 1));
            expect(is_unicode_noncharacter(i));
            expect(is_unicode_noncharacter(i + 1));
            expect(!is_unicode_noncharacter(i + 2));
        }
    });

    return etest::run_all_tests();
}
