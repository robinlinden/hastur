// SPDX-FileCopyrightText: 2021 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "util/string.h"

#include "etest/etest.h"

using namespace std::literals;

using etest::expect;
using etest::expect_eq;
using util::no_case_compare;
using util::trim;
using util::trim_end;
using util::trim_start;

int main() {
    etest::test("no case compare", [] {
        expect(no_case_compare("word"sv, "word"sv));
        expect(no_case_compare("WORD"sv, "WORD"sv));
        expect(no_case_compare("word"sv, "WORD"sv));
        expect(no_case_compare("WORD"sv, "word"sv));
        expect(no_case_compare("Abc-Def_Ghi"sv, "aBc-DEf_gHi"sv));
        expect(no_case_compare("10 seconds"sv, "10 Seconds"sv));
        expect(no_case_compare("Abc $#@"sv, "ABC $#@"sv));
        expect(!no_case_compare(" word"sv, "word"sv));
        expect(!no_case_compare("word "sv, "word"sv));
        expect(!no_case_compare("word "sv, "woord"sv));
    });

    etest::test("trim start", [] {
        expect_eq(trim_start(" abc "), "abc ");
        expect_eq(trim_start("\t431\r\n"), "431\r\n");
        expect_eq(trim_start("  hello world!"), "hello world!");
        expect_eq(trim_start("word "), "word ");
        expect_eq(trim_start("\r\n"), "");
    });

    etest::test("trim end", [] {
        expect_eq(trim_end("abc "), "abc");
        expect_eq(trim_end("53 \r\n"), "53");
        expect_eq(trim_end("hello world!\t"), "hello world!");
        expect_eq(trim_end(" word"), " word");
        expect_eq(trim_end("\r\n"), "");
    });

    etest::test("trim", [] {
        expect_eq(trim("abc"), "abc");
        expect_eq(trim("\t431"), "431");
        expect_eq(trim("53 \r\n"), "53");
        expect_eq(trim("\t\thello world\n"), "hello world");
        expect_eq(trim(" a b c d "), "a b c d");
        expect_eq(trim("\r\n"), "");
    });

    etest::test("trim with non-ascii characters", [] {
        expect_eq(trim("Ö"), "Ö");
        expect_eq(trim(" Ö "), "Ö");
        expect_eq(trim_start(" Ö "), "Ö ");
        expect_eq(trim_end(" Ö "), " Ö");
    });

    return etest::run_all_tests();
}
