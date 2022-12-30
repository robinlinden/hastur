// SPDX-FileCopyrightText: 2023 David Zero <zero-one@zer0-one.net>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "util/uuid.h"

#include "etest/etest.h"

#include <iostream>
#include <string>

using etest::require;

int main() {
    etest::test("UUID formatting", [] {
        std::string u = util::new_uuid();

        std::cout << "Generated UUID: " << u << "\n";

        require(u.size() == 36);
        require(u[8] == '-');
        require(u[13] == '-');
        require(u[18] == '-');
        require(u[23] == '-');
    });

    // To-Do(dzero): Add collision test

    return etest::run_all_tests();
}
