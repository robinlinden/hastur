// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "etest/etest.h"

int main() {
    etest::test("this should fail", [] { etest::expect_eq(1, 2); });
    return etest::run_all_tests();
}
