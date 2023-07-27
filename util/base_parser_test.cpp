// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "util/base_parser.h"

#include "etest/etest.h"

using etest::expect;
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

    return etest::run_all_tests();
}
