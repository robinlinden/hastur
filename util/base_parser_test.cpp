// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "util/base_parser.h"

#include "etest/etest.h"

using etest::expect;
using etest::expect_eq;
using util::BaseParser;

int main() {
    etest::test("peek", [] {
        constexpr auto kAbcd = BaseParser("abcd");
        expect(kAbcd.peek() == 'a');
        expect(kAbcd.peek(2) == "ab");
        expect(kAbcd.peek(3) == "abc");
        expect(kAbcd.peek(4) == "abcd");
        expect(BaseParser(" ").peek() == ' ');
    });

    etest::test("starts_with", [] {
        constexpr auto kAbcd = BaseParser("abcd");
        expect(!kAbcd.starts_with("hello"));
        expect(kAbcd.starts_with("ab"));
        expect(kAbcd.starts_with("abcd"));
    });

    etest::test("is_eof, advance", [] {
        constexpr auto kAbcd = BaseParser("abcd");
        expect(!kAbcd.is_eof());
        expect(BaseParser("").is_eof());

        auto p = BaseParser("abcd");
        p.advance(3);
        expect(!p.is_eof());

        p.advance(1);
        expect(p.is_eof());
    });

    etest::test("consume_char", [] {
        auto p = BaseParser("abcd");
        expect_eq(p.consume_char(), 'a');
        expect_eq(p.consume_char(), 'b');
        expect_eq(p.consume_char(), 'c');
        expect_eq(p.consume_char(), 'd');
    });

    etest::test("consume_while", [] {
        auto p = BaseParser("abcd");
        expect_eq(p.consume_while([](char c) { return c != 'c'; }), "ab");
        expect_eq(p.consume_while([](char c) { return c != 'd'; }), "c");
    });

    etest::test("skip_whitespace, consume_char", [] {
        auto p = BaseParser("      \t       \n         h          \n\n\ni  ");
        p.skip_whitespace();
        expect_eq(p.consume_char(), 'h');

        p.skip_whitespace();
        expect_eq(p.consume_char(), 'i');

        p.skip_whitespace();
        expect(p.is_eof());
    });
}
