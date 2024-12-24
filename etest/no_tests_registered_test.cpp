// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "etest/etest2.h"

// If you try to run tests, but none will run due to them all being filtered out
// or something, that's probably an error.
int main() {
    bool failure = etest::Suite{}.run() != 0;
    return failure ? 0 : 1;
}
