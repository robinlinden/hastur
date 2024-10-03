// SPDX-FileCopyrightText: 2022-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "util/history.h"

#include "etest/etest2.h"

#include <optional>
#include <vector>

using util::History;

int main() {
    etest::Suite s;
    s.add_test("no history", [](etest::IActions &a) {
        History<int> h;
        a.expect_eq(h.current(), std::nullopt);
        a.expect_eq(h.next(), std::nullopt);
        a.expect_eq(h.previous(), std::nullopt);
        a.expect_eq(h.pop(), std::nullopt);
    });

    s.add_test("no history", [](etest::IActions &a) {
        History<int> h;

        h.push(1);
        a.expect_eq(h.current(), 1);
        a.expect_eq(h.next(), std::nullopt);
        a.expect_eq(h.previous(), std::nullopt);

        h.push(2);
        a.expect_eq(h.current(), 2);
        a.expect_eq(h.next(), std::nullopt);
        a.expect_eq(h.previous(), 1);
    });

    s.add_test("popping", [](etest::IActions &a) {
        History<int> h;

        h.push(1);
        h.push(2);

        a.expect_eq(h.pop(), 2);
        a.expect_eq(h.current(), 1);
        a.expect_eq(h.next(), 2);
        a.expect_eq(h.previous(), std::nullopt);

        a.expect_eq(h.pop(), 1);
        a.expect_eq(h.current(), std::nullopt);
        a.expect_eq(h.next(), 1);
        a.expect_eq(h.previous(), std::nullopt);

        a.expect_eq(h.pop(), std::nullopt);
        a.expect_eq(h.current(), std::nullopt);
        a.expect_eq(h.next(), 1);
        a.expect_eq(h.previous(), std::nullopt);
    });

    s.add_test("rewriting history", [](etest::IActions &a) {
        History<int> h;

        h.push(1);
        h.push(2);
        h.push(3);
        h.push(4);

        a.expect_eq(h.pop(), 4);
        a.expect_eq(h.pop(), 3);
        h.push(5);

        a.expect_eq(h.current(), 5);
        a.expect_eq(h.next(), std::nullopt);
        a.expect_eq(h.previous(), 2);
        a.expect_eq(h.entries(), std::vector<int>{1, 2, 5});
    });

    s.add_test("duplicate entries aren't added", [](etest::IActions &a) {
        History<int> h;

        h.push(1);
        h.push(1);

        a.expect_eq(h.current(), 1);
        a.expect_eq(h.next(), std::nullopt);
        a.expect_eq(h.previous(), std::nullopt);
        a.expect_eq(h.entries(), std::vector<int>{1});
    });

    s.add_test("pushing an entry already in history doesn't clear entries after it", [](etest::IActions &a) {
        History<int> h;

        h.push(1);
        h.push(2);
        h.push(3);
        h.push(4);
        a.expect_eq(h.entries(), std::vector<int>{1, 2, 3, 4});
        a.expect_eq(h.pop(), 4);
        a.expect_eq(h.pop(), 3);
        a.expect_eq(h.pop(), 2);

        a.expect_eq(h.entries(), std::vector<int>{1, 2, 3, 4});

        h.push(2);
        a.expect_eq(h.entries(), std::vector<int>{1, 2, 3, 4});
    });

    return s.run();
}
