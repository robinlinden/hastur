// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "util/from_chars.h"

#include "etest/etest.h"

#include <string_view>

using namespace std::literals;
using etest::expect_eq;

int main() {
    etest::test("success", [] {
        auto from = "100.5"sv;
        float v{};
        auto res = util::from_chars(from.data(), from.data() + from.size(), v);
        expect_eq(res, util::from_chars_result{from.data() + from.size(), std::errc{}});
        expect_eq(v, 100.5f);
    });

    etest::test("success, negative", [] {
        auto from = "-100.5"sv;
        float v{};
        auto res = util::from_chars(from.data(), from.data() + from.size(), v);
        expect_eq(res, util::from_chars_result{from.data() + from.size(), std::errc{}});
        expect_eq(v, -100.5f);
    });

    etest::test("failure, out of range", [] {
        auto from = "1e100000"sv;
        float v{};
        auto res = util::from_chars(from.data(), from.data() + from.size(), v);
        expect_eq(res, util::from_chars_result{from.data() + from.size(), std::errc::result_out_of_range});
#ifndef _MSC_VER
        // Microsoft's STL sets v to HUGE_VALF when ERANGE occurs.
        // See: https://en.cppreference.com/w/cpp/utility/from_chars#Return_value
        expect_eq(v, 0.f);
#endif
    });

    etest::test("failure, not a float", [] {
        auto from = "abcd"sv;
        float v{};
        auto res = util::from_chars(from.data(), from.data() + from.size(), v);
        expect_eq(res, util::from_chars_result{from.data(), std::errc::invalid_argument});
        expect_eq(v, 0.f);
    });

    return etest::run_all_tests();
}
