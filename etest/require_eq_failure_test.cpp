// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "etest/etest.h"

#include <cstdlib>

int main() {
    etest::test("this should fail", [] {
        etest::require_eq(1, 2);
        std::exit(1); // Exit w/ failure if this line runs.
    });

    // Invert to return success on failure.
    return !etest::run_all_tests();
}
