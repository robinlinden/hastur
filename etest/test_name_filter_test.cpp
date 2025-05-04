// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "etest/etest2.h"

int main() {
    auto s = etest::Suite{};
    s.add_test("good 1", [](auto &) {});
    s.add_test("good 2", [](auto &) {});
    s.add_test("good 3", [](auto &) {});
    s.add_test("BAD (not good)", [](auto &a) { a.require(false); });
    return s.run({.test_name_filter = "^good"});
}
