// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "gfx/font.h"

#include "etest/etest.h"

using etest::expect;
using etest::expect_eq;

int main() {
    etest::test("used as a bitset", [] {
        auto style = gfx::FontStyle::Normal;
        expect((style & gfx::FontStyle::Italic) != gfx::FontStyle::Italic);

        style |= gfx::FontStyle::Italic;
        expect((style & gfx::FontStyle::Italic) == gfx::FontStyle::Italic);

        style &= ~gfx::FontStyle::Italic;
        expect((style & gfx::FontStyle::Italic) != gfx::FontStyle::Italic);

        expect_eq(style, gfx::FontStyle::Normal);
    });

    return etest::run_all_tests();
}
