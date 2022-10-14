// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef ETEST_TEST_H_
#define ETEST_TEST_H_

#include "etest/cxx_compat.h"

#include <concepts>
#include <functional>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace etest {

template<typename T>
concept Printable = requires(std::ostream &os, T t) {
    { os << t } -> std::same_as<std::ostream &>;
};

[[nodiscard]] int run_all_tests() noexcept;
void test(std::string name, std::function<void()> body) noexcept;
void disabled_test(std::string name, std::function<void()> body) noexcept;

// Weak test requirement. Allows the test to continue even if the check fails.
void expect(bool,
        std::optional<std::string_view> log_message = std::nullopt,
        etest::source_location const &loc = etest::source_location::current()) noexcept;

// Hard test requirement. Stops the test (using an exception) if the check fails.
void require(bool,
        std::optional<std::string_view> log_message = std::nullopt,
        etest::source_location const &loc = etest::source_location::current());

// Weak test requirement. Prints the types compared on failure (if printable).
template<Printable T, Printable U>
void expect_eq(T const &a,
        U const &b,
        std::optional<std::string_view> log_message = std::nullopt,
        etest::source_location const &loc = etest::source_location::current()) noexcept {
    std::stringstream ss;
    ss << a << " !=\n" << b;
    expect(a == b, log_message ? std::move(log_message) : std::move(ss).str(), loc);
}

template<typename T, typename U>
void expect_eq(T const &a,
        U const &b,
        std::optional<std::string_view> log_message = std::nullopt,
        etest::source_location const &loc = etest::source_location::current()) noexcept {
    expect(a == b, std::move(log_message), loc);
}

// Hard test requirement. Prints the types compared on failure (if printable).
template<Printable T, Printable U>
void require_eq(T const &a,
        U const &b,
        std::optional<std::string_view> log_message = std::nullopt,
        etest::source_location const &loc = etest::source_location::current()) {
    std::stringstream ss;
    ss << a << " !=\n" << b;
    require(a == b, log_message ? std::move(log_message) : std::move(ss).str(), loc);
}

template<typename T, typename U>
void require_eq(T const &a,
        U const &b,
        std::optional<std::string_view> log_message = std::nullopt,
        etest::source_location const &loc = etest::source_location::current()) {
    require(a == b, std::move(log_message), loc);
}

} // namespace etest

#endif
