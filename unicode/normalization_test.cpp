// SPDX-FileCopyrightText: 2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "unicode/normalization.h"

#include "etest/etest2.h"

int main() {
    etest::Suite s{};

    s.add_test("not decomposed", [](etest::IActions &a) {
        a.expect_eq(unicode::Normalization::decompose("abc123xyz"), "abc123xyz"); //
    });

    s.add_test("decomposed", [](etest::IActions &a) {
        // A + COMBINING RING ABOVE
        a.expect_eq(unicode::Normalization::decompose("Å"), "A\xcc\x8a");

        // s + COMBINING DOT BELOW + COMBINING DOT ABOVE
        a.expect_eq(unicode::Normalization::decompose("ṩ"), "s\xcc\xa3\xcc\x87");
    });

    s.add_test("mixed", [](etest::IActions &a) {
        // s + COMBINING DOT BELOW + COMBINING DOT ABOVE
        a.expect_eq(unicode::Normalization::decompose("123ṩ567"),
                "123"
                "s\xcc\xa3\xcc\x87"
                "567");
    });

    return s.run();
}
