// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "gfx/color.h"

#include "etest/etest.h"

#include <optional>

using etest::expect;
using etest::expect_eq;
using gfx::Color;

int main() {
    etest::test("Color::from_rgb", [] {
        expect(Color{0x12, 0x34, 0x56} == Color::from_rgb(0x12'34'56));
        expect(Color{} == Color::from_rgb(0));
        expect(Color{0xFF, 0xFF, 0xFF} == Color::from_rgb(0xFF'FF'FF));
    });

    etest::test("Color::from_rgba", [] {
        expect(Color{0x12, 0x34, 0x56, 0x78} == Color::from_rgba(0x12'34'56'78));
        expect(Color{.a = 0x00} == Color::from_rgba(0));
        expect(Color{0xFF, 0xFF, 0xFF, 0xFF} == Color::from_rgba(0xFF'FF'FF'FF));
        expect(Color{0xFF, 0xFF, 0xFF, 0x00} == Color::from_rgba(0xFF'FF'FF'00));
    });

    // Some of the HSL test-cases are from or inspired by
    // https://github.com/web-platform-tests/wpt/blob/0bbb3104a8bc5381d3974adf4535fa0dfe191060/css/css-color/parsing/color-computed-hsl.html
    etest::test("Color::from_hsl", [] {
        expect_eq(Color::from_hsl(120.f, 1.f, 0.25f), Color{.g = 0x80});

        expect_eq(Color::from_hsl(120.f, .3f, .5f), Color{89, 166, 89});
        expect_eq(Color::from_hsl(0.f, 0.f, 0.f), Color{0, 0, 0});
        expect_eq(Color::from_hsl(0.f, 1.f, .5f), Color{255, 0, 0});
        expect_eq(Color::from_hsl(120.f, 0.f, 0.f), Color{0, 0, 0});
        expect_eq(Color::from_hsl(120.f, 0.f, .5f), Color{128, 128, 128});
        expect_eq(Color::from_hsl(120.f, 1.f, .5f), Color{0, 255, 0});
        expect_eq(Color::from_hsl(120.f, .3f, .5f), Color{89, 166, 89});
        expect_eq(Color::from_hsl(120.f, .8f, 0), Color{0, 0, 0});

        expect_eq(Color::from_hsl(300.f, .5f, .5f), Color{191, 64, 191});
        expect_eq(Color::from_hsl(60.f, 1.00f, .375f), Color{191, 191, 0});
        expect_eq(Color::from_hsl(30.f, 1.f, 1.f), Color{255, 255, 255});

        // Angles are represented as a part of a circle and wrap around.
        expect_eq(Color::from_hsl(-300.f, 1.f, .375f), Color{191, 191, 0});
        expect_eq(Color::from_hsl(780.f, 1.f, .375f), Color{191, 191, 0});
    });

    etest::test("Color::from_hsl/a", [] {
        expect_eq(Color::from_hsla(0.f, 0.f, 0.f, 0.f), Color{0, 0, 0, 0});
        expect_eq(Color::from_hsla(0.f, 0.f, 0.f, 0.5f), Color{0, 0, 0, 128});
        expect_eq(Color::from_hsla(120.f, .3f, .5f, .5f), Color{89, 166, 89, 128});
        expect_eq(Color::from_hsla(30.f, 1.f, 1.f, 1.f), Color{255, 255, 255});

        // Angles are represented as a part of a circle and wrap around.
        // Invalid alpha values should be clamped to 0 and 1 respectively
        expect_eq(Color::from_hsla(-300.f, 1.f, .375f, -3.f), Color{191, 191, 0, 0});
        expect_eq(Color::from_hsla(-300.f, 1.f, .375f, 0.f), Color{191, 191, 0, 0});
        expect_eq(Color::from_hsla(-300.f, 1.f, .375f, .2f), Color{191, 191, 0, 51});
        expect_eq(Color::from_hsla(-300.f, 1.f, .375f, 1.f), Color{191, 191, 0});
        expect_eq(Color::from_hsla(-300.f, 1.f, .375f, 12.f), Color{191, 191, 0});
    });

    etest::test("Color::from_css_name", [] {
        expect_eq(Color::from_css_name("blue"), Color{.b = 0xFF});
        expect_eq(Color::from_css_name("not a valid css name"), std::nullopt);
    });

    etest::test("Color::as_rgba_u32", [] {
        expect_eq(Color{0x12, 0x34, 0x56}.as_rgba_u32(), 0x12'34'56'FFu);
        expect_eq(Color{0x12, 0x34, 0x56, 0x78}.as_rgba_u32(), 0x12'34'56'78u);

        auto c = Color{0x12, 0x34, 0x56, 0x78};
        expect_eq(Color::from_rgba(c.as_rgba_u32()), c);
    });

    return etest::run_all_tests();
}
