// SPDX-FileCopyrightText: 2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "etest/etest2.h"

#include <iostream>
#include <random>
#include <tuple>

int main() {
    unsigned seed = std::random_device{}();
    etest::Suite s;

    int last_run_test{};
    s.add_test("1", [&](auto &) { last_run_test = 1; });
    s.add_test("2", [&](auto &) { last_run_test = 2; });

    std::ignore = s.run({.rng_seed = seed});
    auto after_first_run = last_run_test;

    std::ignore = s.run({.rng_seed = seed});
    if (last_run_test != after_first_run) {
        std::cerr << "Tests didn't run in the same order with the same seed\n";
        return 1;
    }
}
