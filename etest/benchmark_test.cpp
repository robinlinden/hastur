// SPDX-FileCopyrightText: 2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "etest/etest2.h"

int main() {
    etest::Suite s{};

    s.add_test("empty benchmark filter", [](etest::IActions &a) {
        etest::Suite filtered{};
        filtered.add_test("empty benchmark filter", [](etest::IActions &) {});
        filtered.add_benchmark("unselected benchmark", [] {});

        a.expect_eq(filtered.run({.test_name_filter = "^empty benchmark filter$"}), 0);
    });

    s.add_benchmark("hello", [] {
        etest::Suite suite_constructor_i_guess{}; //
    });

    return s.run();
}
