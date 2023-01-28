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
