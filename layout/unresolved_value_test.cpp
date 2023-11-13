// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "layout/unresolved_value.h"

#include "etest/etest2.h"

int main() {
    etest::Suite s{"UnresolvedValue"};

    s.add_test("unit/px", [](etest::IActions &a) {
        // Just a raw numeric value.
        auto const uv = layout::UnresolvedValue{.raw = "37px"};
        a.expect_eq(uv.resolve(100, 100), 37);
        a.expect_eq(uv.resolve(123, 456), 37);
        a.expect_eq(uv.resolve(0, 0), 37);
    });

    s.add_test("unit/em", [](etest::IActions &a) {
        // Based on the first argument, the current element's font-size.
        auto const uv = layout::UnresolvedValue{.raw = "2em"};
        a.expect_eq(uv.resolve(100, 100), 200);
        a.expect_eq(uv.resolve(123, 456), 246);
        a.expect_eq(uv.resolve(0, 0), 0);
    });

    s.add_test("unit/rem", [](etest::IActions &a) {
        // Based on the second argument, the root element's font-size.
        auto const uv = layout::UnresolvedValue{.raw = "2rem"};
        a.expect_eq(uv.resolve(100, 100), 200);
        a.expect_eq(uv.resolve(123, 456), 912);
        a.expect_eq(uv.resolve(0, 0), 0);
    });

    return s.run();
}
