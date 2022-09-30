// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "etest/etest.h"

using etest::expect;

int main() {
    etest::test("this should fail", [] { expect(false); });
    return etest::run_all_tests();
}
