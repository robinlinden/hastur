// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "util/history.h"

#include "etest/etest.h"

#include <optional>
#include <vector>

using etest::expect_eq;
using util::History;

int main() {
    etest::test("no history", [] {
        History<int> h;
        expect_eq(h.current(), std::nullopt);
        expect_eq(h.next(), std::nullopt);
        expect_eq(h.previous(), std::nullopt);
        expect_eq(h.pop(), std::nullopt);
    });

    etest::test("pushing", [] {
        History<int> h;

        h.push(1);
        expect_eq(h.current(), 1);
        expect_eq(h.next(), std::nullopt);
        expect_eq(h.previous(), std::nullopt);

        h.push(2);
        expect_eq(h.current(), 2);
        expect_eq(h.next(), std::nullopt);
        expect_eq(h.previous(), 1);
    });

    etest::test("popping", [] {
        History<int> h;

        h.push(1);
        h.push(2);

        expect_eq(h.pop(), 2);
        expect_eq(h.current(), 1);
        expect_eq(h.next(), 2);
        expect_eq(h.previous(), std::nullopt);

        expect_eq(h.pop(), 1);
        expect_eq(h.current(), std::nullopt);
        expect_eq(h.next(), 1);
        expect_eq(h.previous(), std::nullopt);

        expect_eq(h.pop(), std::nullopt);
        expect_eq(h.current(), std::nullopt);
        expect_eq(h.next(), 1);
        expect_eq(h.previous(), std::nullopt);
    });

    etest::test("rewriting history", [] {
        History<int> h;

        h.push(1);
        h.push(2);
        h.push(3);
        h.push(4);

        expect_eq(h.pop(), 4);
        expect_eq(h.pop(), 3);
        h.push(5);

        expect_eq(h.current(), 5);
        expect_eq(h.next(), std::nullopt);
        expect_eq(h.previous(), 2);
        expect_eq(h.entries(), std::vector<int>{1, 2, 5});
    });

    etest::test("duplicate entries aren't added", [] {
        History<int> h;

        h.push(1);
        h.push(1);

        expect_eq(h.current(), 1);
        expect_eq(h.next(), std::nullopt);
        expect_eq(h.previous(), std::nullopt);
        expect_eq(h.entries(), std::vector<int>{1});
    });

    etest::test("pushing an entry already in history doesn't clear entries after it", [] {
        History<int> h;

        h.push(1);
        h.push(2);
        h.push(3);
        h.push(4);
        expect_eq(h.entries(), std::vector<int>{1, 2, 3, 4});
        expect_eq(h.pop(), 4);
        expect_eq(h.pop(), 3);
        expect_eq(h.pop(), 2);

        expect_eq(h.entries(), std::vector<int>{1, 2, 3, 4});

        h.push(2);
        expect_eq(h.entries(), std::vector<int>{1, 2, 3, 4});
    });

    return etest::run_all_tests();
}
