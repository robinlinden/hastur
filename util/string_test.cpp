// SPDX-FileCopyrightText: 2021 Mikael Larsson <c.mikael.larsson@gmail.com>
// SPDX-FileCopyrightText: 2022-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "util/string.h"

#include "etest/etest2.h"

#include <climits>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

using namespace std::literals;
using namespace util;

int main() {
    etest::Suite es;
    es.add_test("is_c0", [](etest::IActions &a) {
#if CHAR_MIN < 0
        // I would use constexpr-if here, but we still get warnings about c < 0
        // always being false when char is unsigned unless we preprocess this
        // out.
        for (char c = std::numeric_limits<char>::min(); c < 0; ++c) {
            a.expect(!is_c0(c));
        }
#endif
        for (char c = 0; c < 0x20; ++c) {
            a.expect(is_c0(c));
        }
        for (char c = 0x20; c < std::numeric_limits<char>::max(); ++c) {
            a.expect(!is_c0(c));
        }
    });

    es.add_test("is_upper_alpha", [](etest::IActions &a) {
        a.expect(is_upper_alpha('A'));
        a.expect(!is_upper_alpha('a'));
    });

    es.add_test("is_lower_alpha", [](etest::IActions &a) {
        a.expect(is_lower_alpha('a'));
        a.expect(!is_lower_alpha('A'));
    });

    es.add_test("is_alpha", [](etest::IActions &a) {
        a.expect(is_alpha('a'));
        a.expect(!is_alpha('!'));
    });

    es.add_test("is_digit", [](etest::IActions &a) {
        a.expect(is_digit('0'));
        a.expect(!is_digit('a'));
    });

    es.add_test("is_alphanumeric", [](etest::IActions &a) {
        a.expect(is_alphanumeric('a'));
        a.expect(!is_alphanumeric('!'));
    });

    es.add_test("is_upper_hex_digit", [](etest::IActions &a) {
        a.expect(is_upper_hex_digit('F'));
        a.expect(!is_upper_hex_digit('f'));
    });

    es.add_test("is_lower_hex_digit", [](etest::IActions &a) {
        a.expect(is_lower_hex_digit('f'));
        a.expect(!is_lower_hex_digit('F'));
    });

    es.add_test("is_hex_digit", [](etest::IActions &a) {
        a.expect(is_hex_digit('f'));
        a.expect(!is_hex_digit('!'));
    });

    es.add_test("is_punctuation", [](etest::IActions &a) {
        a.expect(is_punctuation('!'));
        a.expect(!is_punctuation('a'));
    });

    es.add_test("is_printable", [](etest::IActions &a) {
        a.expect(is_printable('a'));
        a.expect(is_printable(' '));
        a.expect(is_printable('!'));
        a.expect(!is_printable('\n'));
    });

    es.add_test("lowercased(char)", [](etest::IActions &a) {
        a.expect_eq(lowercased('A'), 'a');
        a.expect_eq(lowercased('a'), 'a');
    });

    es.add_test("lowercased(std::string)", [](etest::IActions &a) {
        a.expect_eq(lowercased("Hello There!!1"), "hello there!!1");
        a.expect_eq(lowercased("woop woop"), "woop woop");
    });

    es.add_test("no case compare", [](etest::IActions &a) {
        a.expect(no_case_compare("word"sv, "word"sv));
        a.expect(no_case_compare("WORD"sv, "WORD"sv));
        a.expect(no_case_compare("word"sv, "WORD"sv));
        a.expect(no_case_compare("WORD"sv, "word"sv));
        a.expect(no_case_compare("Abc-Def_Ghi"sv, "aBc-DEf_gHi"sv));
        a.expect(no_case_compare("10 seconds"sv, "10 Seconds"sv));
        a.expect(no_case_compare("Abc $#@"sv, "ABC $#@"sv));
        a.expect(!no_case_compare(" word"sv, "word"sv));
        a.expect(!no_case_compare("word "sv, "word"sv));
        a.expect(!no_case_compare("word "sv, "woord"sv));
    });

    es.add_test("split, single char delimiter", [](etest::IActions &a) {
        std::string_view str{"a,b,c,d"};
        auto s = split(str, ",");
        a.require(s.size() == 4);
        a.expect_eq(s[0], "a");
        a.expect_eq(s[1], "b");
        a.expect_eq(s[2], "c");
        a.expect_eq(s[3], "d");
    });

    es.add_test("split, multi char delimiter", [](etest::IActions &a) {
        std::string_view str{"abbbcbbbdbbbe"};
        auto s = split(str, "bbb");
        a.require(s.size() == 4);
        a.expect_eq(s[0], "a");
        a.expect_eq(s[1], "c");
        a.expect_eq(s[2], "d");
        a.expect_eq(s[3], "e");
    });

    es.add_test("split, empty between delimiter", [](etest::IActions &a) {
        std::string_view str{"name;;age;address"};
        auto s = split(str, ";");
        a.require(s.size() == 4);
        a.expect_eq(s[0], "name");
        a.expect_eq(s[1], "");
        a.expect_eq(s[2], "age");
        a.expect_eq(s[3], "address");
    });

    es.add_test("split, delimiter at start", [](etest::IActions &a) {
        std::string_view str{";a;b;c"};
        auto s = split(str, ";");
        a.require(s.size() == 4);
        a.expect_eq(s[0], "");
        a.expect_eq(s[1], "a");
        a.expect_eq(s[2], "b");
        a.expect_eq(s[3], "c");
    });

    es.add_test("split, delimiter at end", [](etest::IActions &a) {
        std::string_view str{"a;b;c;"};
        auto s = split(str, ";");
        a.require(s.size() == 4);
        a.expect_eq(s[0], "a");
        a.expect_eq(s[1], "b");
        a.expect_eq(s[2], "c");
        a.expect_eq(s[3], "");
    });

    es.add_test("split, only delimiter", [](etest::IActions &a) {
        std::string_view str{";"};
        auto s = split(str, ";");
        a.require(s.size() == 2);
        a.expect_eq(s[0], "");
        a.expect_eq(s[1], "");
    });

    es.add_test("split, empty string", [](etest::IActions &a) {
        std::string_view str;
        auto s = split(str, ";");
        a.require(s.size() == 1);
        a.expect_eq(s[0], "");
    });

    es.add_test("split, multi char delimiter at start and end", [](etest::IActions &a) {
        std::string_view str{"bbbabbbcbbbdbbbebbb"};
        auto s = split(str, "bbb");
        a.require(s.size() == 6);
        a.expect_eq(s[0], "");
        a.expect_eq(s[1], "a");
        a.expect_eq(s[2], "c");
        a.expect_eq(s[3], "d");
        a.expect_eq(s[4], "e");
        a.expect_eq(s[5], "");
    });

    es.constexpr_test("split once, single char delimiter", [](etest::IActions &a) {
        std::string_view str{"a,b,c,d"};
        auto p = split_once(str, ',');
        a.expect_eq(p.first, "a");
        a.expect_eq(p.second, "b,c,d");
    });

    es.constexpr_test("split once, delimiter at start", [](etest::IActions &a) {
        std::string_view str{",a"};
        auto p = split_once(str, ',');
        a.expect_eq(p.first, "");
        a.expect_eq(p.second, "a");
    });

    es.constexpr_test("split once, delimiter at end", [](etest::IActions &a) {
        std::string_view str{"a,"};
        auto p = split_once(str, ',');
        a.expect_eq(p.first, "a");
        a.expect_eq(p.second, "");
    });

    es.constexpr_test("split once, only delimiter", [](etest::IActions &a) {
        std::string_view str{","};
        auto p = split_once(str, ',');
        a.expect_eq(p.first, "");
        a.expect_eq(p.second, "");
    });

    es.add_test("is whitespace", [](etest::IActions &a) {
        a.expect(is_whitespace(' '));
        a.expect(is_whitespace('\n'));
        a.expect(is_whitespace('\r'));
        a.expect(is_whitespace('\f'));
        a.expect(is_whitespace('\v'));
        a.expect(is_whitespace('\t'));

        a.expect(!is_whitespace('a'));
        a.expect(!is_whitespace('\0'));
    });

    es.add_test("trim start", [](etest::IActions &a) {
        a.expect_eq(trim_start(" abc "sv), "abc "sv);
        a.expect_eq(trim_start("\t431\r\n"sv), "431\r\n"sv);
        a.expect_eq(trim_start("  hello world!"sv), "hello world!"sv);
        a.expect_eq(trim_start("word "sv), "word "sv);
        a.expect_eq(trim_start("\r\n"sv), ""sv);
    });

    es.add_test("trim start, should_trim", [](etest::IActions &a) {
        a.expect_eq(trim_start(" abc "sv, [](char c) { return c == ' '; }), "abc "sv);
        a.expect_eq(trim_start(" abc "sv, [](char c) { return c <= 'a'; }), "bc "sv);
        a.expect_eq(trim_start(" abc "sv, [](char c) { return c <= 'b'; }), "c "sv);
        a.expect_eq(trim_start(" abc "sv, [](char c) { return c <= 'c'; }), ""sv);
        a.expect_eq(trim_start(" abc "sv, [](char c) { return c == '\t'; }), " abc "sv);
    });

    es.add_test("trim end", [](etest::IActions &a) {
        a.expect_eq(trim_end("abc "sv), "abc"sv);
        a.expect_eq(trim_end("53 \r\n"sv), "53"sv);
        a.expect_eq(trim_end("hello world!\t"sv), "hello world!"sv);
        a.expect_eq(trim_end(" word"sv), " word"sv);
        a.expect_eq(trim_end("\r\n"sv), ""sv);
    });

    es.add_test("trim end, should_trim", [](etest::IActions &a) {
        a.expect_eq(trim_end(" abc "sv, [](char c) { return c == ' '; }), " abc"sv);
        a.expect_eq(trim_end(" abc "sv, [](char c) { return c <= 'c'; }), ""sv);
        a.expect_eq(trim_end(" abc "sv, [](char c) { return c == ' ' || c >= 'b'; }), " a"sv);
        a.expect_eq(trim_end(" abc "sv, [](char c) { return c == '\t'; }), " abc "sv);
    });

    es.add_test("trim", [](etest::IActions &a) {
        a.expect_eq(trim("abc"sv), "abc"sv);
        a.expect_eq(trim("\t431"sv), "431"sv);
        a.expect_eq(trim("53 \r\n"sv), "53"sv);
        a.expect_eq(trim("\t\thello world\n"sv), "hello world"sv);
        a.expect_eq(trim(" a b c d "sv), "a b c d"sv);
        a.expect_eq(trim("\r\n"sv), ""sv);
    });

    es.add_test("trim, should_trim", [](etest::IActions &a) {
        a.expect_eq(trim("abcba"sv, [](char c) { return c == ' '; }), "abcba"sv);
        a.expect_eq(trim("abcba"sv, [](char c) { return c <= 'a'; }), "bcb"sv);
        a.expect_eq(trim("abcba"sv, [](char c) { return c <= 'b'; }), "c"sv);
        a.expect_eq(trim("abcba"sv, [](char c) { return c <= 'c'; }), ""sv);
    });

    es.add_test("trim with non-ascii characters", [](etest::IActions &a) {
        a.expect_eq(trim("Ö"sv), "Ö"sv);
        a.expect_eq(trim(" Ö "sv), "Ö"sv);
        a.expect_eq(trim_start(" Ö "sv), "Ö "sv);
        a.expect_eq(trim_end(" Ö "sv), " Ö"sv);
    });

    es.add_test("join: empty", [](etest::IActions &a) {
        std::vector<std::string_view> strings;
        a.expect_eq(join(strings, ","sv), ""sv);
    });

    es.add_test("join: 1 item", [](etest::IActions &a) {
        std::vector<std::string_view> strings{"a"};
        a.expect_eq(join(strings, ","sv), "a"sv);
    });

    es.add_test("join: 2 items", [](etest::IActions &a) {
        std::vector<std::string_view> strings{"a", "b"};
        a.expect_eq(join(strings, ","sv), "a,b"sv);
    });

    return es.run();
}
