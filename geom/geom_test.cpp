// SPDX-FileCopyrightText: 2021-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "geom/geom.h"

#include "etest/etest2.h"

using geom::EdgeSize;
using geom::Position;
using geom::Rect;

int main() {
    etest::Suite s{};

    s.add_test("Position::scaled", [](etest::IActions &a) {
        Position p{0, 0};

        a.expect_eq(Position{}, p.scaled(0));
        a.expect_eq(p, p.scaled(1));
        a.expect_eq(Position{0, 0}, p.scaled(2));
        a.expect_eq(Position{0, 0}, p.scaled(3));

        Position r1{1, 1};
        a.expect_eq(r1, r1.scaled(1));
        a.expect_eq(Position{2, 2}, r1.scaled(2));
        a.expect_eq(Position{3, 3}, r1.scaled(3));

        Position r2{1, 1};
        a.expect_eq(r2, r2.scaled(1, {1, 1}));
        a.expect_eq(Position{1, 1}, r2.scaled(2, {1, 1}));
        a.expect_eq(Position{1, 1}, r2.scaled(3, {1, 1}));

        Position r3{0, 0};
        a.expect_eq(r3, r3.scaled(1, {5, 5}));
        a.expect_eq(Position{-5, -5}, r3.scaled(2, {5, 5}));
        a.expect_eq(Position{-10, -10}, r3.scaled(3, {5, 5}));
    });

    s.add_test("Position::translated", [](etest::IActions &a) {
        Position p{0, 0};
        a.expect_eq(Position{10, 0}, p.translated(10, 0));
        a.expect_eq(Position{0, 10}, p.translated(0, 10));
        a.expect_eq(Position{-10, -10}, p.translated(-10, -10));
    });

    s.add_test("Rect::position", [](etest::IActions &a) {
        a.expect_eq(Rect{-10, 0, 20, 10}.position(), Position{-10, 0});
        a.expect_eq(Rect{0, 0, 20, 10}.position(), Position{0, 0});
        a.expect_eq(Rect{10, 10, 5, 5}.position(), Position{10, 10});
    });

    s.add_test("Rect::expanded", [](etest::IActions &a) {
        Rect r{0, 0, 10, 10};
        a.expect(Rect{-10, 0, 20, 10} == r.expanded(EdgeSize{10, 0, 0, 0}));
        a.expect(Rect{0, 0, 20, 10} == r.expanded(EdgeSize{0, 10, 0, 0}));
        a.expect(Rect{0, -10, 10, 20} == r.expanded(EdgeSize{0, 0, 10, 0}));
        a.expect(Rect{0, 0, 10, 20} == r.expanded(EdgeSize{0, 0, 0, 10}));

        a.expect(Rect{-10, 0, 30, 10} == r.expanded(EdgeSize{10, 10, 0, 0}));
        a.expect(Rect{0, -10, 10, 30} == r.expanded(EdgeSize{0, 0, 10, 10}));
        a.expect(Rect{0, 0, 20, 20} == r.expanded(EdgeSize{0, 10, 0, 10}));

        a.expect(Rect{-10, -10, 30, 30} == r.expanded(EdgeSize{10, 10, 10, 10}));
    });

    s.add_test("Rect::scaled", [](etest::IActions &a) {
        Rect r{0, 0, 10, 10};

        a.expect(Rect{} == r.scaled(0));
        a.expect(r == r.scaled(1));
        a.expect(Rect{0, 0, 20, 20} == r.scaled(2));
        a.expect(Rect{0, 0, 30, 30} == r.scaled(3));

        Rect r1{1, 1, 10, 10};
        a.expect(r1 == r1.scaled(1));
        a.expect(Rect{2, 2, 20, 20} == r1.scaled(2));
        a.expect(Rect{3, 3, 30, 30} == r1.scaled(3));

        Rect r2{1, 1, 10, 10};
        a.expect(r2 == r2.scaled(1, {1, 1}));
        a.expect(Rect{1, 1, 20, 20} == r2.scaled(2, {1, 1}));
        a.expect(Rect{1, 1, 30, 30} == r2.scaled(3, {1, 1}));

        Rect r3{0, 0, 10, 10};
        a.expect(r3 == r3.scaled(1, {5, 5}));
        a.expect(Rect{-5, -5, 20, 20} == r3.scaled(2, {5, 5}));
        a.expect(Rect{-10, -10, 30, 30} == r3.scaled(3, {5, 5}));
    });

    s.add_test("Rect::translated", [](etest::IActions &a) {
        Rect r{0, 0, 10, 10};
        a.expect(Rect{10, 0, 10, 10} == r.translated(10, 0));
        a.expect(Rect{0, 10, 10, 10} == r.translated(0, 10));
        a.expect(Rect{-10, -10, 10, 10} == r.translated(-10, -10));
    });

    s.add_test("Rect::intersected", [](etest::IActions &a) {
        Rect r{0, 0, 10, 10};

        // Intersect with self should be a no-op.
        a.expect_eq(r, r.intersected(r));

        a.expect_eq(Rect{3, 4, 5, 5}, r.intersected({3, 4, 5, 5}));
        a.expect_eq(Rect{0, 0, 1, 2}, r.intersected({0, 0, 1, 2}));
        a.expect_eq(Rect{8, 5, 2, 5}, r.intersected({8, 5, 10, 10}));
        a.expect_eq(Rect{0, 0, 2, 2}, r.intersected({-2, -2, 4, 4}));

        a.expect_eq(Rect{-10, -10, 5, 5}, Rect{-20, -20, 15, 15}.intersected({-10, -10, 100, 100}));

        // Intersect with a non-overlapping rect should yield an empty rect.
        a.expect_eq(Rect{}, r.intersected({-1, -1, 1, 1}));
        a.expect_eq(Rect{}, r.intersected({11, 11, 1, 1}));
    });

    s.add_test("Rect::contains", [](etest::IActions &a) {
        Rect r{0, 0, 10, 10};
        a.expect(r.contains({0, 0}));
        a.expect(r.contains({0, 10}));
        a.expect(r.contains({10, 10}));
        a.expect(r.contains({10, 0}));
        a.expect(r.contains({5, 5}));
        a.expect(!r.contains({-1, 0}));
        a.expect(!r.contains({-1, 10}));
        a.expect(!r.contains({10, 11}));
        a.expect(!r.contains({11, 10}));
    });

    s.add_test("Rect::empty", [](etest::IActions &a) {
        a.expect(Rect{0, 0, 0, 0}.empty());
        a.expect(Rect{0, 0, 1, 0}.empty());
        a.expect(Rect{0, 0, 0, 1}.empty());
        a.expect(!Rect{0, 0, 1, 1}.empty());
    });

    return s.run();
}
