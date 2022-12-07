// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "etest/etest.h"

#include <iostream>

int main() {
    etest::disabled_test("hi", [] { etest::expect(false); });

    if (etest::run_all_tests() != 0) {
        std::cerr << "A disabled test ran when it shouldn't have\n";
        return 1;
    }
}
