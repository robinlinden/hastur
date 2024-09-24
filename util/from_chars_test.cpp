// SPDX-FileCopyrightText: 2022-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "util/from_chars.h"

#include "etest/etest2.h"

#include <string_view>
#include <system_error>

using namespace std::literals;

int main() {
    etest::Suite s;

    s.add_test("success", [](etest::IActions &a) {
        auto from = "100.5"sv;
        float v{};
        auto res = util::from_chars(from.data(), from.data() + from.size(), v);
        a.expect_eq(res, util::from_chars_result{from.data() + from.size(), std::errc{}});
        a.expect_eq(v, 100.5f);
    });

    s.add_test("success, negative", [](etest::IActions &a) {
        auto from = "-100.5"sv;
        float v{};
        auto res = util::from_chars(from.data(), from.data() + from.size(), v);
        a.expect_eq(res, util::from_chars_result{from.data() + from.size(), std::errc{}});
        a.expect_eq(v, -100.5f);
    });

    s.add_test("failure, out of range", [](etest::IActions &a) {
        auto from = "1e100000"sv;
        float v{};
        auto res = util::from_chars(from.data(), from.data() + from.size(), v);
        a.expect_eq(res, util::from_chars_result{from.data() + from.size(), std::errc::result_out_of_range});
#ifndef _MSC_VER
        // Microsoft's STL sets v to HUGE_VALF when ERANGE occurs.
        // See: https://en.cppreference.com/w/cpp/utility/from_chars#Return_value
        a.expect_eq(v, 0.f);
#endif
    });

    s.add_test("failure, not a float", [](etest::IActions &a) {
        auto from = "abcd"sv;
        float v{};
        auto res = util::from_chars(from.data(), from.data() + from.size(), v);
        a.expect_eq(res, util::from_chars_result{from.data(), std::errc::invalid_argument});
        a.expect_eq(v, 0.f);
    });

    return s.run();
}
