// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "etest/etest.h"

using etest::require;

int main() {
    etest::test("this should fail", [] { require(false); });
    return etest::run_all_tests();
}
