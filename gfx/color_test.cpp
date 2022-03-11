// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "gfx/color.h"

#include "etest/etest.h"

using etest::expect;
using etest::expect_eq;
using gfx::Color;

int main() {
    etest::test("Color::from_rgb", [] {
        expect(Color{0x12, 0x34, 0x56} == Color::from_rgb(0x12'34'56));
        expect(Color{} == Color::from_rgb(0));
        expect(Color{0xFF, 0xFF, 0xFF} == Color::from_rgb(0xFF'FF'FF));
    });

    etest::test("Color::as_rgba_u32", [] {
        expect_eq(Color{0x12, 0x34, 0x56}.as_rgba_u32(), 0x12'34'56'FFu);
        expect_eq(Color{0x12, 0x34, 0x56, 0x78}.as_rgba_u32(), 0x12'34'56'78u);
    });

    return etest::run_all_tests();
}
