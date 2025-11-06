// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "etest/etest2.h"

// TODO(robinlinden): Add a way of catching the test output so it can be
// properly tested. This just hits the code paths, but doesn't verify the
// output.
int main() {
    etest::Suite s{};
    s.add_test("expect(true)", [](etest::IActions &a) { a.expect(true); });
    s.add_test("expect(false)", [](etest::IActions &a) { a.expect(false); });
    int ret = s.run({.enable_color_output = false});

    // We expect the suite to fail. That's fine.
    return ret == 0 ? 1 : 0;
}
