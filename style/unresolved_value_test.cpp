// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "style/unresolved_value.h"

#include "etest/etest2.h"

#include <optional>

using style::UnresolvedValue;

int main() {
    etest::Suite s{"UnresolvedValue"};

    s.add_test("unit/px", [](etest::IActions &a) {
        // Just a raw numeric value.
        auto const uv = UnresolvedValue{.raw = "37px"};
        a.expect_eq(uv.resolve(100, {.root_font_size = 100}), 37);
        a.expect_eq(uv.resolve(123, {.root_font_size = 456}), 37);
        a.expect_eq(uv.resolve(0, {.root_font_size = 0}), 37);
    });

    s.add_test("unit/em", [](etest::IActions &a) {
        // Based on the first argument, the current element's font-size.
        auto const uv = UnresolvedValue{.raw = "2em"};
        a.expect_eq(uv.resolve(100, {.root_font_size = 100}), 200);
        a.expect_eq(uv.resolve(123, {.root_font_size = 456}), 246);
        a.expect_eq(uv.resolve(0, {.root_font_size = 0}), 0);
    });

    s.add_test("unit/ex", [](etest::IActions &a) {
        // Based on the first argument, the current element's font-size.
        auto const uv = UnresolvedValue{.raw = "1ex"};
        a.expect_eq(uv.resolve(100, {.root_font_size = 100}), 50);
        a.expect_eq(uv.resolve(123, {.root_font_size = 456}), 61);
        a.expect_eq(uv.resolve(0, {.root_font_size = 0}), 0);
    });

    s.add_test("unit/ch", [](etest::IActions &a) {
        // Based on the first argument, the current element's font-size.
        auto const uv = UnresolvedValue{.raw = "1ch"};
        a.expect_eq(uv.resolve(100, {.root_font_size = 100}), 50);
        a.expect_eq(uv.resolve(123, {.root_font_size = 456}), 61);
        a.expect_eq(uv.resolve(0, {.root_font_size = 0}), 0);
    });

    s.add_test("unit/rem", [](etest::IActions &a) {
        // Based on the second argument, the root element's font-size.
        auto const uv = UnresolvedValue{.raw = "2rem"};
        a.expect_eq(uv.resolve(100, {.root_font_size = 100}), 200);
        a.expect_eq(uv.resolve(123, {.root_font_size = 456}), 912);
        a.expect_eq(uv.resolve(0, {.root_font_size = 0}), 0);
    });

    s.add_test("unit/%", [](etest::IActions &a) {
        // Based on the third argument, whatever the spec wants the property
        // this came from to be resolved against.
        auto const uv = UnresolvedValue{.raw = "50%"};
        a.expect_eq(uv.resolve(100, {.root_font_size = 100}, 100), 50);
        a.expect_eq(uv.resolve(100, {.root_font_size = 100}, 200), 100);
        a.expect_eq(uv.resolve(0, {.root_font_size = 0}, 1000), 500);

        // If the third argument is not provided, you get nothing.
        a.expect_eq(uv.resolve(123, {.root_font_size = 456}), 0);
    });

    s.add_test("try_resolve", [](etest::IActions &a) {
        // %, no parent provided.
        auto const percent = UnresolvedValue{.raw = "50%"};
        a.expect_eq(percent.try_resolve(100, {.root_font_size = 100}), std::nullopt);
        a.expect_eq(percent.try_resolve(100, {.root_font_size = 100}, 100), 50);

        // Nonsense.
        auto const nonsense = UnresolvedValue{.raw = "foo"};
        a.expect_eq(nonsense.try_resolve(100, {.root_font_size = 100}, 100), std::nullopt);
    });

    return s.run();
}
