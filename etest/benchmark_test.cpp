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

    s.add_test("filtered benchmarks", [](etest::IActions &a) {
        etest::Suite filtered{};
        bool selected_ran = false;
        bool unselected_ran = false;
        filtered.add_benchmark("filtered benchmarks", [&] { selected_ran = true; });
        filtered.add_benchmark("unselected benchmark", [&] { unselected_ran = true; });

        a.expect_eq(filtered.run({.test_name_filter = "^filtered benchmarks$"}), 0);
        a.expect(selected_ran);
        a.expect(!unselected_ran);
    });

    s.add_benchmark("hello", [] {
        etest::Suite suite_constructor_i_guess{}; //
    });

    return s.run();
}
