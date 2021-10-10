// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "etest/etest.h"

int main() {
    etest::test("this should fail", [] { etest::expect_eq(1, 2); });

    // Invert to return success on failure.
    return !etest::run_all_tests();
}
