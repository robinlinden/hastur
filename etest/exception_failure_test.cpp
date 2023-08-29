// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "etest/etest2.h"

int main() {
    auto s = etest::Suite{};
    s.add_test("uncaught exception", []([[maybe_unused]] auto &a) {
#if defined(__EXCEPTIONS) || defined(_MSC_VER)
        throw std::exception{};
#else
        a.require(false);
#endif
    });
    return s.run();
}
