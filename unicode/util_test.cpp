// SPDX-FileCopyrightText: 2022-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "unicode/util.h"

#include "etest/etest2.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using namespace std::literals;
using namespace unicode;

int main() {
    etest::Suite s{};

    s.add_test("utf8_byte_count", [](etest::IActions &a) {
        a.expect_eq(utf8_byte_count(0), 1);
        a.expect_eq(utf8_byte_count(0x7f), 1);

        a.expect_eq(utf8_byte_count(0x80), 2);
        a.expect_eq(utf8_byte_count(0x7ff), 2);

        a.expect_eq(utf8_byte_count(0x800), 3);
        a.expect_eq(utf8_byte_count(0xffff), 3);

        a.expect_eq(utf8_byte_count(0x100000), 4);
        a.expect_eq(utf8_byte_count(0x10ffff), 4);

        a.expect_eq(utf8_byte_count(0x110000), std::nullopt);
    });

    s.add_test("to_utf8", [](etest::IActions &a) {
        a.expect_eq(to_utf8(0x002f), "/"sv);

        a.expect_eq(to_utf8(0x00a3), "£"sv);
        a.expect_eq(to_utf8(0x07f9), "߹"sv);

        a.expect_eq(to_utf8(0x0939), "ह"sv);
        a.expect_eq(to_utf8(0x20ac), "€"sv);
        a.expect_eq(to_utf8(0xd55c), "한"sv);
        a.expect_eq(to_utf8(0xfffd), "�"sv);

        a.expect_eq(to_utf8(0x10348), "𐍈"sv);

        // Invalid code points return "".
        a.expect_eq(to_utf8(0x110000), ""sv);
    });

    s.add_test("is_surrogate", [](etest::IActions &a) {
        a.expect(!is_surrogate(0xD799));
        a.expect(is_surrogate(0xD800)); // First leading surrogate.
        a.expect(is_surrogate(0xDBFF)); // Last leading surrogate.
        a.expect(is_surrogate(0xDC00)); // First trailing surrogate.
        a.expect(is_surrogate(0xDFFF)); // Last trailing surrogate.
        a.expect(!is_surrogate(0xE000));
    });

    s.add_test("is_noncharacter", [](etest::IActions &a) {
        a.expect(!is_noncharacter(0xFDD0 - 1));

        for (std::uint32_t i = 0xFDD0; i <= 0xFDEF; ++i) {
            a.expect(is_noncharacter(i));
        }

        a.expect(!is_noncharacter(0xFDEF + 1));
        a.expect(!is_noncharacter(0xFFFE - 1));

        // Every 0x10000 pair of values ending in FFFE and FFFF are noncharacters.
        for (std::uint32_t i = 0xFFFE; i <= 0x10FFFE; i += 0x10000) {
            a.expect(!is_noncharacter(i - 1));
            a.expect(is_noncharacter(i));
            a.expect(is_noncharacter(i + 1));
            a.expect(!is_noncharacter(i + 2));
        }
    });

    s.add_test("utf8_to_utf32", [](etest::IActions &a) {
        a.expect_eq(utf8_to_utf32("/"sv), 0x002ful);

        a.expect_eq(utf8_to_utf32("Д"sv), 0x0414ul);

        a.expect_eq(utf8_to_utf32("ᛋ"sv), 0x16cbul);

        a.expect_eq(utf8_to_utf32("🫸"sv), 0x1faf8ul);

        // Pass several codepoints, it should just decode the first one
        a.expect_eq(utf8_to_utf32("🯷🯷🯷"sv), 0x1fbf7ul);
    });

    s.add_test("utf8_length", [](etest::IActions &a) {
        a.expect_eq(utf8_length("🮻"sv), 1ul);
        a.expect_eq(utf8_length("This string is 33 characters long"sv), 33ul);
        a.expect_eq(utf8_length("🤖🤖🤖"sv), 3ul);
        a.expect_eq(utf8_length("🆒🆒🆒🆒🆒🆒🆒!"sv), 8ul);

        // First byte suggests a 2-byte char, but we don't supply the 2nd byte
        std::string invalid{static_cast<char>(0b11000000)};
        a.expect_eq(utf8_length(invalid), std::nullopt);
    });

    s.add_test("CodePointView", [](etest::IActions &a) {
        auto into_code_points = [](std::string_view sv) {
            std::vector<std::uint32_t> code_points{};
            for (auto cp : CodePointView{sv}) {
                code_points.push_back(cp);
            }
            return code_points;
        };

        // 3x ROBOT FACE
        a.expect_eq(into_code_points("🤖🤖🤖"sv), std::vector<std::uint32_t>{0x1f916, 0x1f916, 0x1f916});

        // GOTHIC LETTER HWAIR.
        a.expect_eq(into_code_points("\xf0\x90\x8d\x88"sv), std::vector<std::uint32_t>{0x10348});

        // Boring ASCII.
        a.expect_eq(into_code_points("abcd"sv), std::vector<std::uint32_t>{'a', 'b', 'c', 'd'});

        // REGISTERED SIGN
        a.expect_eq(into_code_points("\xc2\xae"sv), std::vector<std::uint32_t>{0xae});

        // BUGINESE END OF SECTION
        a.expect_eq(into_code_points("\xe1\xa8\x9f"sv), std::vector<std::uint32_t>{0x1a1f});
    });

    return s.run();
}
