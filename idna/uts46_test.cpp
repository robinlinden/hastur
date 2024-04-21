// SPDX-FileCopyrightText: 2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "idna/uts46.h"

#include "etest/etest2.h"

#include <optional>
#include <string_view>

using namespace std::literals;

// https://unicode.org/reports/tr46/#Table_Example_Processing
int main() {
    etest::Suite s{};

    s.add_test("disallowed", [](etest::IActions &a) {
        a.expect_eq(idna::Uts46::map("\0"sv), std::nullopt);
        a.expect_eq(idna::Uts46::map(","), std::nullopt);
        a.expect_eq(idna::Uts46::map("\xc2\xa0"), std::nullopt);
        a.expect_eq(idna::Uts46::map("a⒈com"), std::nullopt);
    });

    s.add_test("mapped", [](etest::IActions &a) {
        a.expect_eq(idna::Uts46::map("ABCXYZ"), "abcxyz");
        a.expect_eq(idna::Uts46::map("日本語。ＪＰ"), "日本語.jp");
        a.expect_eq(idna::Uts46::map("☕.us"), "☕.us");
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
