// SPDX-FileCopyrightText: 2021 Mikael Larsson <c.mikael.larsson@gmail.com>
// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "util/string.h"

#include "etest/etest.h"

using namespace std::literals;
using namespace util;

using etest::expect;
using etest::expect_eq;
using etest::require;

int main() {
    etest::test("is_upper_alpha", [] {
        expect(is_upper_alpha('A'));
        expect(!is_upper_alpha('a'));
    });

    etest::test("is_lower_alpha", [] {
        expect(is_lower_alpha('a'));
        expect(!is_lower_alpha('A'));
    });

    etest::test("is_alpha", [] {
        expect(is_alpha('a'));
        expect(!is_alpha('!'));
    });

    etest::test("is_digit", [] {
        expect(is_digit('0'));
        expect(!is_digit('a'));
    });

    etest::test("is_alphanumeric", [] {
        expect(is_alphanumeric('a'));
        expect(!is_alphanumeric('!'));
    });

    etest::test("is_upper_hex_digit", [] {
        expect(is_upper_hex_digit('F'));
        expect(!is_upper_hex_digit('f'));
    });

    etest::test("is_lower_hex_digit", [] {
        expect(is_lower_hex_digit('f'));
        expect(!is_lower_hex_digit('F'));
    });

    etest::test("is_hex_digit", [] {
        expect(is_hex_digit('f'));
        expect(!is_hex_digit('!'));
    });

    etest::test("to_lower", [] {
        expect_eq(to_lower('A'), 'a');
        expect_eq(to_lower('a'), 'a');
    });

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

    etest::test("is whitespace", [] {
        expect(is_whitespace(' '));
        expect(is_whitespace('\n'));
        expect(is_whitespace('\r'));
        expect(is_whitespace('\f'));
        expect(is_whitespace('\v'));
        expect(is_whitespace('\t'));

        expect(!is_whitespace('a'));
        expect(!is_whitespace('\0'));
    });

    etest::test("trim start", [] {
        expect_eq(trim_start(" abc "sv), "abc "sv);
        expect_eq(trim_start("\t431\r\n"sv), "431\r\n"sv);
        expect_eq(trim_start("  hello world!"sv), "hello world!"sv);
        expect_eq(trim_start("word "sv), "word "sv);
        expect_eq(trim_start("\r\n"sv), ""sv);
    });

    etest::test("trim end", [] {
        expect_eq(trim_end("abc "sv), "abc"sv);
        expect_eq(trim_end("53 \r\n"sv), "53"sv);
        expect_eq(trim_end("hello world!\t"sv), "hello world!"sv);
        expect_eq(trim_end(" word"sv), " word"sv);
        expect_eq(trim_end("\r\n"sv), ""sv);
    });

    etest::test("trim", [] {
        expect_eq(trim("abc"sv), "abc"sv);
        expect_eq(trim("\t431"sv), "431"sv);
        expect_eq(trim("53 \r\n"sv), "53"sv);
        expect_eq(trim("\t\thello world\n"sv), "hello world"sv);
        expect_eq(trim(" a b c d "sv), "a b c d"sv);
        expect_eq(trim("\r\n"sv), ""sv);
    });

    etest::test("trim with non-ascii characters", [] {
        expect_eq(trim("Ö"sv), "Ö"sv);
        expect_eq(trim(" Ö "sv), "Ö"sv);
        expect_eq(trim_start(" Ö "sv), "Ö "sv);
        expect_eq(trim_end(" Ö "sv), " Ö"sv);
    });

    return etest::run_all_tests();
}
