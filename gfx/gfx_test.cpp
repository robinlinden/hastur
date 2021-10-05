// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "gfx/gfx.h"

#include "etest/etest.h"

using etest::expect;
using gfx::Color;

int main() {
    etest::test("Color::from_rgb", [] {
        expect(Color{0x12, 0x34, 0x56} == Color::from_rgb(0x12'34'56));
        expect(Color{} == Color::from_rgb(0));
        expect(Color{0xFF, 0xFF, 0xFF} == Color::from_rgb(0xFF'FF'FF));
    });

    return etest::run_all_tests();
}
