// SPDX-FileCopyrightText: 2023 David Zero <zero-one@zer0-one.net>
// SPDX-FileCopyrightText: 2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "util/uuid.h"

#include "etest/etest2.h"

#include <iostream>
#include <string>

int main() {
    etest::Suite s;

    s.add_test("UUID formatting", [](etest::IActions &a) {
        std::string u = util::new_uuid();

        std::cout << "Generated UUID: " << u << "\n";

        a.require(u.size() == 36);
        a.require(u[8] == '-');
        a.require(u[13] == '-');
        a.require(u[18] == '-');
        a.require(u[23] == '-');
    });

    // To-Do(dzero): Add collision test

    return s.run();
}
