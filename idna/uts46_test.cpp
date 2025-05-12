// SPDX-FileCopyrightText: 2024-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "idna/uts46.h"

#include "etest/etest2.h"

#include <optional>

// https://unicode.org/reports/tr46/#Table_Example_Processing
int main() {
    etest::Suite s{};

    s.add_test("disallowed", [](etest::IActions &a) {
        // The first disallowed unicode value.
        a.expect_eq(idna::Uts46::map("\xc2\x80"), std::nullopt); // U+0080
        // and the last one, U+10FFFF, but in UTF-8.
        a.expect_eq(idna::Uts46::map("\xf4\x8f\xbf\xbf"), std::nullopt);

        a.expect_eq(idna::Uts46::map("\xc2\x9f"), std::nullopt); // Application program command.
        a.expect_eq(idna::Uts46::map("a⒈com"), std::nullopt);
    });

    s.add_test("mapped", [](etest::IActions &a) {
        a.expect_eq(idna::Uts46::map("\xc2\xa0"), " "); // No-break space.
        a.expect_eq(idna::Uts46::map("ABCXYZ"), "abcxyz");
        a.expect_eq(idna::Uts46::map("日本語。ＪＰ"), "日本語.jp");
        a.expect_eq(idna::Uts46::map("☕.us"), "☕.us");

        // Code point that maps to a character requiring 5 characters to
        // represent, \u{20A2C}.
        // https://www.compart.com/en/unicode/U+2F834
        a.expect_eq(idna::Uts46::map("\xf0\xaf\xa0\xb4").value(), "\xf0\xa0\xa8\xac");
    });

    s.add_test("deviation", [](etest::IActions &a) {
        a.expect_eq(idna::Uts46::map("Bloß.de"), "bloß.de");
        a.expect_eq(idna::Uts46::map("BLOẞ.de"), "bloß.de");
    });

    s.add_test("ignored", [](etest::IActions &a) {
        a.expect_eq(idna::Uts46::map("\xc2\xad"), ""); //
    });

    return s.run();
}
