// SPDX-FileCopyrightText: 2021-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "etest/etest2.h"

int main() {
    etest::Suite s{};
    s.add_test("expect", [](etest::IActions &a) { a.expect(true); });
    s.add_test("expect_eq", [](etest::IActions &a) { a.expect_eq(1, 1); });
    s.add_test("require", [](etest::IActions &a) { a.require(true); });
    s.add_test("require_eq", [](etest::IActions &a) { a.require_eq(1, 1); });
    return s.run();
}
