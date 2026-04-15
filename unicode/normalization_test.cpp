// SPDX-FileCopyrightText: 2024-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "unicode/normalization.h"

#include "etest/etest2.h"

int main() {
    etest::Suite s{};

    s.add_test("nfd: ascii, unchanged", [](etest::IActions &a) {
        a.expect_eq(unicode::Normalization::nfd("abc123xyz"), "abc123xyz"); //
    });

    s.add_test("nfd: needs decomposition", [](etest::IActions &a) {
        // A + COMBINING RING ABOVE
        a.expect_eq(unicode::Normalization::nfd("Å"), "A\xcc\x8a");

        // s + COMBINING DOT BELOW + COMBINING DOT ABOVE
        a.expect_eq(unicode::Normalization::nfd("ṩ"), "s\xcc\xa3\xcc\x87");
    });

    s.add_test("nfd: ascii mixed w/ decomposable", [](etest::IActions &a) {
        // s + COMBINING DOT BELOW + COMBINING DOT ABOVE
        a.expect_eq(unicode::Normalization::nfd("123ṩ567"),
                "123"
                "s\xcc\xa3\xcc\x87"
                "567");
    });

    s.add_test("nfd: needs reordering", [](etest::IActions &a) {
        // q + COMBINING CIRCUMFLEX ACCENT + COMBINING DOT BELOW
        // The circumflex has combining class 230, and the dot has 220, so the
        // dot should be sorted before the circumflex.
        a.expect_eq(unicode::Normalization::nfd("q\xCC\x82\xCC\xA3"), "q\xCC\xA3\xCC\x82");
    });

    return s.run();
}
