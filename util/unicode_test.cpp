// SPDX-FileCopyrightText: 2022-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "util/unicode.h"

#include "etest/etest.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

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

        for (std::uint32_t i = 0xFDD0; i <= 0xFDEF; ++i) {
            expect(is_unicode_noncharacter(i));
        }

        expect(!is_unicode_noncharacter(0xFDEF + 1));
        expect(!is_unicode_noncharacter(0xFFFE - 1));

        // Every 0x10000 pair of values ending in FFFE and FFFF are noncharacters.
        for (std::uint32_t i = 0xFFFE; i <= 0x10FFFE; i += 0x10000) {
            expect(!is_unicode_noncharacter(i - 1));
            expect(is_unicode_noncharacter(i));
            expect(is_unicode_noncharacter(i + 1));
            expect(!is_unicode_noncharacter(i + 2));
        }
    });

    etest::test("utf8_to_utf32", [] {
        expect_eq(utf8_to_utf32("/"sv), 0x002ful);

        expect_eq(utf8_to_utf32("Д"sv), 0x0414ul);

        expect_eq(utf8_to_utf32("ᛋ"sv), 0x16cbul);

        expect_eq(utf8_to_utf32("🫸"sv), 0x1faf8ul);

        // Pass several codepoints, it should just decode the first one
        expect_eq(utf8_to_utf32("🯷🯷🯷"sv), 0x1fbf7ul);
    });

    etest::test("utf8_length", [] {
        expect_eq(utf8_length("🮻"sv), 1ul);
        expect_eq(utf8_length("This string is 33 characters long"sv), 33ul);
        expect_eq(utf8_length("🤖🤖🤖"sv), 3ul);
        expect_eq(utf8_length("🆒🆒🆒🆒🆒🆒🆒!"sv), 8ul);

        // First byte suggests a 2-byte char, but we don't supply the 2nd byte
        std::string invalid{static_cast<char>(0b11000000)};
        expect_eq(utf8_length(invalid), std::nullopt);
    });

    etest::test("CodePointView", [] {
        auto into_code_points = [](std::string_view s) {
            std::vector<std::uint32_t> code_points{};
            for (auto cp : CodePointView{s}) {
                code_points.push_back(cp);
            }
            return code_points;
        };

        // 3x ROBOT FACE
        expect_eq(into_code_points("🤖🤖🤖"sv), std::vector<std::uint32_t>{0x1f916, 0x1f916, 0x1f916});

        // GOTHIC LETTER HWAIR.
        expect_eq(into_code_points("\xf0\x90\x8d\x88"sv), std::vector<std::uint32_t>{0x10348});

        // Boring ASCII.
        expect_eq(into_code_points("abcd"sv), std::vector<std::uint32_t>{'a', 'b', 'c', 'd'});

        // REGISTERED SIGN
        expect_eq(into_code_points("\xc2\xae"sv), std::vector<std::uint32_t>{0xae});

        // BUGINESE END OF SECTION
        expect_eq(into_code_points("\xe1\xa8\x9f"sv), std::vector<std::uint32_t>{0x1a1f});
    });

    return etest::run_all_tests();
}
