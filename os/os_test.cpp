// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "os/os.h"

#include "etest/etest.h"

using etest::expect;

int main() {
    etest::test("font_paths", [] {
        auto font_paths = os::font_paths();
        expect(!font_paths.empty());
    });

    return etest::run_all_tests();
}
