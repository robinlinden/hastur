// SPDX-FileCopyrightText: 2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "etest/etest2.h"

int main() {
    etest::Suite s{};

    s.add_benchmark("hello", [] {
        etest::Suite suite_constructor_i_guess{}; //
    });

    return s.run();
}
