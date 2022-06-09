// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "util/base_parser.h"

#include "etest/etest.h"

using etest::expect;
using util::BaseParser;

int main() {
    etest::test("peek", [] {
        constexpr auto abcd = BaseParser("abcd");
        expect(abcd.peek() == 'a');
        expect(abcd.peek(2) == "ab");
        expect(abcd.peek(3) == "abc");
        expect(abcd.peek(4) == "abcd");
        expect(BaseParser(" ").peek() == ' ');
    });

    etest::test("starts_with", [] {
        constexpr auto abcd = BaseParser("abcd");
        expect(!abcd.starts_with("hello"));
        expect(abcd.starts_with("ab"));
        expect(abcd.starts_with("abcd"));
    });

    etest::test("is_eof, advance", [] {
        constexpr auto abcd = BaseParser("abcd");
        expect(!abcd.is_eof());
        expect(BaseParser("").is_eof());
        expect([] {
            auto p = BaseParser("abcd");
            p.advance(3);
            if (p.is_eof()) {
                return false;
            }
            p.advance(1);
            return p.is_eof();
        }());
    });

    etest::test("consume_char", [] {
        expect([] {
            auto p = BaseParser("abcd");
            if (p.consume_char() != 'a') {
                return false;
            }
            if (p.consume_char() != 'b') {
                return false;
            }
            if (p.consume_char() != 'c') {
                return false;
            }
            if (p.consume_char() != 'd') {
                return false;
            }
            return true;
        }());
    });

    etest::test("consume_while", [] {
        expect([] {
            auto p = BaseParser("abcd");
            if (p.consume_while([](char c) { return c != 'c'; }) != "ab") {
                return false;
            }
            if (p.consume_while([](char c) { return c != 'd'; }) != "c") {
                return false;
            }
            return true;
        }());
    });

    etest::test("skip_whitespace, consume_char", [] {
        expect([] {
            auto p = BaseParser("      \t       \n         h          \n\n\ni");
            p.skip_whitespace();
            if (p.consume_char() != 'h') {
                return false;
            }
            p.skip_whitespace();
            if (p.consume_char() != 'i') {
                return false;
            }
            return true;
        }());
    });
}
