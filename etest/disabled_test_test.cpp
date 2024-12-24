// SPDX-FileCopyrightText: 2022-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "etest/etest2.h"

#include <iostream>
#include <tuple>

int main() {
    etest::Suite s{};

    bool ran{};
    s.disabled_test("hi", [&](etest::IActions &) { ran = true; });

    std::ignore = s.run();
    if (ran) {
        std::cerr << "A disabled test ran when it shouldn't have\n";
        return 1;
    }

    std::ignore = s.run({.run_disabled_tests = true});
    if (!ran) {
        std::cerr << "A disabled test didn't run when it should have\n";
        return 1;
    }
}
