// SPDX-FileCopyrightText: 2022-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "etest/etest.h"

#include <iostream>
#include <tuple>

int main() {
    bool ran{};
    etest::disabled_test("hi", [&] { ran = true; });

    std::ignore = etest::run_all_tests();
    if (ran) {
        std::cerr << "A disabled test ran when it shouldn't have\n";
        return 1;
    }

    std::ignore = etest::run_all_tests({.run_disabled_tests = true});
    if (!ran) {
        std::cerr << "A disabled test didn't run when it should have\n";
        return 1;
    }
}
