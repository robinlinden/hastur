// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "etest/etest2.h"

int main() {
    auto s = etest::Suite{};
    // We want e.g. exceptions not derived from std::exception to be registered
    // as failures too.
    s.add_test("uncaught number", []([[maybe_unused]] auto &a) {
#if defined(__EXCEPTIONS) || defined(_MSC_VER)
        throw 42;
#else
        a.require(false);
#endif
    });
    return s.run();
}
