// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "etest/etest.h"

int main() {
    etest::test("expect", [] { etest::expect(true); });
    etest::test("expect_eq", [] { etest::expect_eq(1, 1); });
    etest::test("require", [] { etest::require(true); });
    etest::test("require_eq", [] { etest::require_eq(1, 1); });
    return etest::run_all_tests();
}
