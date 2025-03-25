// SPDX-FileCopyrightText: 2022-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "util/from_chars.h"

#include "etest/etest2.h"

#include <string>
#include <string_view>
#include <system_error>

#ifdef _MSC_VER
#define BROKEN_RESULT_OUT_OF_RANGE
#elif defined(_LIBCPP_VERSION) && _LIBCPP_VERSION >= 200000
#define BROKEN_RESULT_OUT_OF_RANGE
#endif

using namespace std::literals;

namespace {

template<typename T>
void add_tests(etest::Suite &s, std::string_view name_prefix) {
    s.add_test(std::string{name_prefix} + ": failure, out of range", [](etest::IActions &a) {
        auto from = "1e100000"sv;
        T v{};
        auto res = util::from_chars(from.data(), from.data() + from.size(), v);
        a.expect_eq(res,
                util::from_chars_result{from.data() + from.size(), std::errc::result_out_of_range},
                std::make_error_code(res.ec).message());
#ifndef BROKEN_RESULT_OUT_OF_RANGE
        // Microsoft's STL and libc++20 sets v to HUGE_VALF when ERANGE occurs.
        // See: https://en.cppreference.com/w/cpp/utility/from_chars#Return_value
        a.expect_eq(v, T{0.});
#endif
    });

    s.add_test(std::string{name_prefix} + ": failure, not a number", [](etest::IActions &a) {
        auto from = "abcd"sv;
        T v{};
        auto res = util::from_chars(from.data(), from.data() + from.size(), v);
        a.expect_eq(res,
                util::from_chars_result{from.data(), std::errc::invalid_argument},
                std::make_error_code(res.ec).message());
        a.expect_eq(v, T{0.});
    });

    s.add_test(std::string{name_prefix} + ": success", [](etest::IActions &a) {
        auto from = "100.5"sv;
        T v{};
        auto res = util::from_chars(from.data(), from.data() + from.size(), v);
        a.expect_eq(res,
                util::from_chars_result{from.data() + from.size(), std::errc{}},
                std::make_error_code(res.ec).message());
        a.expect_eq(v, T{100.5});
    });

    s.add_test(std::string{name_prefix} + ": success, negative", [](etest::IActions &a) {
        auto from = "-100.5"sv;
        T v{};
        auto res = util::from_chars(from.data(), from.data() + from.size(), v);
        a.expect_eq(res,
                util::from_chars_result{from.data() + from.size(), std::errc{}},
                std::make_error_code(res.ec).message());
        a.expect_eq(v, T{-100.5});
    });
}

} // namespace

int main() {
    etest::Suite s;
    add_tests<float>(s, "float");
    add_tests<double>(s, "double");
    return s.run();
}
