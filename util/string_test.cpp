// SPDX-FileCopyrightText: 2021 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "util/string.h"

#include "etest/etest.h"

using namespace std::literals;

using etest::expect;
using etest::expect_eq;
using etest::require;
using util::no_case_compare;
using util::split;
using util::split_once;
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

    etest::test("split, single char delimiter", [] {
        std::string_view str{"a,b,c,d"};
        auto s = split(str, ",");
        require(s.size() == 4);
        expect_eq(s[0], "a");
        expect_eq(s[1], "b");
        expect_eq(s[2], "c");
        expect_eq(s[3], "d");
    });

    etest::test("split, multi char delimiter", [] {
        std::string_view str{"abbbcbbbdbbbe"};
        auto s = split(str, "bbb");
        require(s.size() == 4);
        expect_eq(s[0], "a");
        expect_eq(s[1], "c");
        expect_eq(s[2], "d");
        expect_eq(s[3], "e");
    });

    etest::test("split, empty between delimiter", [] {
        std::string_view str{"name;;age;address"};
        auto s = split(str, ";");
        require(s.size() == 4);
        expect_eq(s[0], "name");
        expect_eq(s[1], "");
        expect_eq(s[2], "age");
        expect_eq(s[3], "address");
    });

    etest::test("split, delimiter at start", [] {
        std::string_view str{";a;b;c"};
        auto s = split(str, ";");
        require(s.size() == 4);
        expect_eq(s[0], "");
        expect_eq(s[1], "a");
        expect_eq(s[2], "b");
        expect_eq(s[3], "c");
    });

    etest::test("split, delimiter at end", [] {
        std::string_view str{"a;b;c;"};
        auto s = split(str, ";");
        require(s.size() == 4);
        expect_eq(s[0], "a");
        expect_eq(s[1], "b");
        expect_eq(s[2], "c");
        expect_eq(s[3], "");
    });

    etest::test("split, only delimiter", [] {
        std::string_view str{";"};
        auto s = split(str, ";");
        require(s.size() == 2);
        expect_eq(s[0], "");
        expect_eq(s[1], "");
    });

    etest::test("split, empty string", [] {
        std::string_view str{""};
        auto s = split(str, ";");
        require(s.size() == 1);
        expect_eq(s[0], "");
    });

    etest::test("split, multi char delimiter at start and end", [] {
        std::string_view str{"bbbabbbcbbbdbbbebbb"};
        auto s = split(str, "bbb");
        require(s.size() == 6);
        expect_eq(s[0], "");
        expect_eq(s[1], "a");
        expect_eq(s[2], "c");
        expect_eq(s[3], "d");
        expect_eq(s[4], "e");
        expect_eq(s[5], "");
    });

    etest::test("split once, single char delimiter", [] {
        std::string_view str{"a,b,c,d"};
        auto p = split_once(str, ",");
        expect_eq(p.first, "a");
        expect_eq(p.second, "b,c,d");
    });

    etest::test("split once, multi char delimiter", [] {
        std::string_view str{"abcccde"};
        auto p = split_once(str, "ccc");
        expect_eq(p.first, "ab");
        expect_eq(p.second, "de");
    });

    etest::test("split once, delimiter at start", [] {
        std::string_view str{",a"};
        auto p = split_once(str, ",");
        expect_eq(p.first, "");
        expect_eq(p.second, "a");
    });

    etest::test("split once, delimiter at end", [] {
        std::string_view str{"a,"};
        auto p = split_once(str, ",");
        expect_eq(p.first, "a");
        expect_eq(p.second, "");
    });

    etest::test("split once, only delimiter", [] {
        std::string_view str{","};
        auto p = split_once(str, ",");
        expect_eq(p.first, "");
        expect_eq(p.second, "");
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
