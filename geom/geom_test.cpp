// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "geom/geom.h"

#include "etest/etest.h"

using etest::expect;
using geom::EdgeSize;
using geom::Rect;

int main() {
    etest::test("Rect::expanded", [] {
        Rect r{0, 0, 10, 10};
        expect(Rect{-10, 0, 20, 10} == r.expanded(EdgeSize{10, 0, 0, 0}));
        expect(Rect{0, 0, 20, 10} == r.expanded(EdgeSize{0, 10, 0, 0}));
        expect(Rect{0, -10, 10, 20} == r.expanded(EdgeSize{0, 0, 10, 0}));
        expect(Rect{0, 0, 10, 20} == r.expanded(EdgeSize{0, 0, 0, 10}));

        expect(Rect{-10, 0, 30, 10} == r.expanded(EdgeSize{10, 10, 0, 0}));
        expect(Rect{0, -10, 10, 30} == r.expanded(EdgeSize{0, 0, 10, 10}));
        expect(Rect{0, 0, 20, 20} == r.expanded(EdgeSize{0, 10, 0, 10}));

        expect(Rect{-10, -10, 30, 30} == r.expanded(EdgeSize{10, 10, 10, 10}));
    });

    etest::test("Rect::translated", [] {
        Rect r{0, 0, 10, 10};
        expect(Rect{10, 0, 10, 10} == r.translated(10, 0));
        expect(Rect{0, 10, 10, 10} == r.translated(0, 10));
        expect(Rect{-10, -10, 10, 10} == r.translated(-10, -10));
    });

    etest::test("Rect::contains", [] {
        Rect r{0, 0, 10, 10};
        expect(r.contains({0, 0}));
        expect(r.contains({0, 10}));
        expect(r.contains({10, 10}));
        expect(r.contains({10, 0}));
        expect(r.contains({5, 5}));
        expect(!r.contains({-1, 0}));
        expect(!r.contains({-1, 10}));
        expect(!r.contains({10, 11}));
        expect(!r.contains({11, 10}));
    });

    return etest::run_all_tests();
}
