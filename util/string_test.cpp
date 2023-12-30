// SPDX-FileCopyrightText: 2021 Mikael Larsson <c.mikael.larsson@gmail.com>
// SPDX-FileCopyrightText: 2022-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "util/string.h"

#include "etest/etest.h"

#include <array>
#include <climits>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>

using namespace std::literals;
using namespace util;

using etest::expect;
using etest::expect_eq;
using etest::require;

int main() {
    etest::test("is_c0", [] {
#if CHAR_MIN < 0
        // I would use constexpr-if here, but we still get warnings about c < 0
        // always being false when char is unsigned unless we preprocess this
        // out.
        for (char c = std::numeric_limits<char>::min(); c < 0; ++c) {
            expect(!is_c0(c));
        }
#endif
        for (char c = 0; c < 0x20; ++c) {
            expect(is_c0(c));
        }
        for (char c = 0x20; c < std::numeric_limits<char>::max(); ++c) {
            expect(!is_c0(c));
        }
    });

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

    etest::test("lowercased(char)", [] {
        expect_eq(lowercased('A'), 'a');
        expect_eq(lowercased('a'), 'a');
    });

    etest::test("lowercased(std::string)", [] {
        expect_eq(lowercased("Hello There!!1"), "hello there!!1");
        expect_eq(lowercased("woop woop"), "woop woop");
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
        std::string_view str;
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

    etest::test("trim start, should_trim", [] {
        expect_eq(trim_start(" abc "sv, [](char c) { return c == ' '; }), "abc "sv);
        expect_eq(trim_start(" abc "sv, [](char c) { return c <= 'a'; }), "bc "sv);
        expect_eq(trim_start(" abc "sv, [](char c) { return c <= 'b'; }), "c "sv);
        expect_eq(trim_start(" abc "sv, [](char c) { return c <= 'c'; }), ""sv);
        expect_eq(trim_start(" abc "sv, [](char c) { return c == '\t'; }), " abc "sv);
    });

    etest::test("trim end", [] {
        expect_eq(trim_end("abc "sv), "abc"sv);
        expect_eq(trim_end("53 \r\n"sv), "53"sv);
        expect_eq(trim_end("hello world!\t"sv), "hello world!"sv);
        expect_eq(trim_end(" word"sv), " word"sv);
        expect_eq(trim_end("\r\n"sv), ""sv);
    });

    etest::test("trim end, should_trim", [] {
        expect_eq(trim_end(" abc "sv, [](char c) { return c == ' '; }), " abc"sv);
        expect_eq(trim_end(" abc "sv, [](char c) { return c <= 'c'; }), ""sv);
        expect_eq(trim_end(" abc "sv, [](char c) { return c == ' ' || c >= 'b'; }), " a"sv);
        expect_eq(trim_end(" abc "sv, [](char c) { return c == '\t'; }), " abc "sv);
    });

    etest::test("trim", [] {
        expect_eq(trim("abc"sv), "abc"sv);
        expect_eq(trim("\t431"sv), "431"sv);
        expect_eq(trim("53 \r\n"sv), "53"sv);
        expect_eq(trim("\t\thello world\n"sv), "hello world"sv);
        expect_eq(trim(" a b c d "sv), "a b c d"sv);
        expect_eq(trim("\r\n"sv), ""sv);
    });

    etest::test("trim, should_trim", [] {
        expect_eq(trim("abcba"sv, [](char c) { return c == ' '; }), "abcba"sv);
        expect_eq(trim("abcba"sv, [](char c) { return c <= 'a'; }), "bcb"sv);
        expect_eq(trim("abcba"sv, [](char c) { return c <= 'b'; }), "c"sv);
        expect_eq(trim("abcba"sv, [](char c) { return c <= 'c'; }), ""sv);
    });

    etest::test("trim with non-ascii characters", [] {
        expect_eq(trim("Ö"sv), "Ö"sv);
        expect_eq(trim(" Ö "sv), "Ö"sv);
        expect_eq(trim_start(" Ö "sv), "Ö "sv);
        expect_eq(trim_end(" Ö "sv), " Ö"sv);
    });

    etest::test("IPv4 serialization", [] {
        std::uint32_t loopback = 2130706433;
        std::uint32_t global = 134744072;
        std::uint32_t nonroutable = 2886729729;

        std::cout << "Serialized IPv4 Loopback Address: " << util::ipv4_serialize(loopback) << "\n";
        std::cout << "Serialized IPv4 Globally-Routable Address: " << util::ipv4_serialize(global) << "\n";
        std::cout << "Serialized IPv4 RFC1918 Address: " << util::ipv4_serialize(nonroutable) << "\n";

        expect(util::ipv4_serialize(loopback) == "127.0.0.1");
        expect(util::ipv4_serialize(global) == "8.8.8.8");
        expect(util::ipv4_serialize(nonroutable) == "172.16.0.1");
    });

    etest::test("IPv6 serialization", [] {
        std::array<std::uint16_t, 8> const loopback{0, 0, 0, 0, 0, 0, 0, 1};
        std::array<std::uint16_t, 8> global{0x2001, 0xdb8, 0x85a3, 0, 0, 0x8a2e, 0x370, 0x7334};

        std::cout << "Serialized IPv6 Loopback Address: " << util::ipv6_serialize(loopback) << "\n";
        std::cout << "Serialized IPv6 Globally-Routable Address: " << util::ipv6_serialize(global) << "\n";

        expect(util::ipv6_serialize(loopback) == "::1");
        expect(util::ipv6_serialize(global) == "2001:db8:85a3::8a2e:370:7334");
    });

    etest::test("uppercase percent-encoded triplets", [] {
        std::string foo{"https://example.com/%ff"};
        std::string foo2{"%be%ee%ee%ff"};
        std::string foo3;
        std::string foo4{"%"};
        std::string foo5{"%77"};
        std::string foo6{"%EE"};

        expect_eq(percent_encoded_triplets_to_upper(foo), "https://example.com/%FF");
        expect_eq(percent_encoded_triplets_to_upper(foo2), "%BE%EE%EE%FF");
        expect_eq(percent_encoded_triplets_to_upper(foo3), "");
        expect_eq(percent_encoded_triplets_to_upper(foo4), "%");
        expect_eq(percent_encoded_triplets_to_upper(foo5), "%77");
        expect_eq(percent_encoded_triplets_to_upper(foo6), "%EE");
    });

    etest::test("percent-decode URL unreserved", [] {
        std::string foo{"https://example.com/%7e"};
        std::string foo2{"%7e%30%61%2D%2e%5F"};
        std::string foo3;
        std::string foo4{"%"};
        std::string foo5{"%77"};
        std::string foo6{"%7F"};

        expect_eq(percent_decode_unreserved(foo), "https://example.com/~");
        expect_eq(percent_decode_unreserved(foo2), "~0a-._");
        expect_eq(percent_decode_unreserved(foo3), "");
        expect_eq(percent_decode_unreserved(foo4), "%");
        expect_eq(percent_decode_unreserved(foo5), "w");
        expect_eq(percent_decode_unreserved(foo6), "%7F");
    });

    return etest::run_all_tests();
}
