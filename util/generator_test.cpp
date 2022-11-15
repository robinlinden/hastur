// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "util/generator.h"

#include "etest/etest.h"

using etest::expect;
using etest::expect_eq;
using etest::require;

int main() {
    etest::test("next", [] {
        auto gen = []() -> util::Generator<int> {
            int i{0};
            while (true) {
                i += 1;
                co_yield i;
            }
        }();

        expect_eq(gen.next(), 1);
        expect_eq(gen.next(), 2);
        expect_eq(gen.next(), 3);
    });

    etest::test("has_next", [] {
        auto gen = []() -> util::Generator<int> {
            co_yield 1;
        }();

        require(gen.has_next());
        expect_eq(gen.next(), 1);
        expect(!gen.has_next());
    });

    etest::test("has_next with no yields", [] {
        auto gen = []() -> util::Generator<int> {
            co_return;
        }();
        expect(!gen.has_next());
    });

    etest::test("move constructor", [] {
        auto gen1 = []() -> util::Generator<int> {
            co_yield 1;
            co_yield 2;
        }();

        expect_eq(1, gen1.next());
        auto gen2{std::move(gen1)};
        expect_eq(2, gen2.next());
        expect(!gen2.has_next());
    });

    etest::test("move assign", [] {
        auto gen1 = []() -> util::Generator<int> {
            co_yield 1;
            co_yield 2;
        }();

        expect_eq(1, gen1.next());

        auto gen2 = []() -> util::Generator<int> {
            co_yield 5;
        }();

        expect_eq(5, gen2.next());

        gen2 = std::move(gen1);
        expect_eq(2, gen2.next());
        expect(!gen2.has_next());
    });

    // util/generator_test.cpp:84:13: error: explicitly moving variable of type
    // 'util::Generator<int>' to itself [-Werror,-Wself-move]
#ifndef __clang__
    etest::test("move assign to self", [] {
        auto gen = []() -> util::Generator<int> {
            co_yield 1;
            co_yield 2;
        }();

        expect_eq(1, gen.next());

        gen = std::move(gen);
        expect_eq(2, gen.next());
        expect(!gen.has_next());
    });
#endif

    etest::test("range-based for loop", [] {
        auto gen = []() -> util::Generator<char> {
            co_yield 'a';
            co_yield 'b';
            co_yield 'c';
        }();

        char current_expectation = 'a';
        for (auto c : gen) {
            expect_eq(current_expectation, c);
            ++current_expectation;
        }

        expect_eq('d', current_expectation);
    });

    etest::test("boring for loop", [] {
        auto gen = []() -> util::Generator<std::pair<char, int>> {
            co_yield {'a', 2};
        }();

        int iterations{};
        for (auto it = std::begin(gen); it != std::end(gen); ++it) {
            expect_eq(it->first, 'a');
            expect_eq(it->second, 2);
            ++iterations;
        }

        expect_eq(1, iterations);
    });

    return etest::run_all_tests();
}
