// SPDX-FileCopyrightText: 2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "etest/etest2.h"

int main() {
    etest::Suite s{};

    // This only tests success since failures in constexpr tests are detected at
    // compile-time, and we want our code to compile.
    s.constexpr_test("secret meme goes here", [](etest::IActions &a) {
        a.expect(true);
        a.expect_eq(1, 1);
        a.require(true);
        a.require_eq(1, 1);
    });

    return s.run();
}
