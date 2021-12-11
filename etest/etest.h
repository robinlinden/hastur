// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef ETEST_TEST_H_
#define ETEST_TEST_H_

#include "etest/cxx_compat.h"

#include <concepts>
#include <functional>
#include <ostream>
#include <string_view>

namespace etest {

template<typename T>
concept Printable = requires(std::ostream &os, T t) {
    { os << t } -> std::same_as<std::ostream &>;
};

int run_all_tests() noexcept;
void test(std::string_view name, std::function<void()> body) noexcept;

// Weak test requirement. Allows the test to continue even if the check fails.
void expect(bool, etest::source_location const &loc = etest::source_location::current()) noexcept;

// Hard test requirement. Stops the test (using an exception) if the check fails.
void require(bool, etest::source_location const &loc = etest::source_location::current());

// Access the internal test log.
std::ostream &log();

// Weak test requirement. Prints the types compared on failure (if printable).
template<Printable T, Printable U>
void expect_eq(T const &a, U const &b, etest::source_location const &loc = etest::source_location::current()) noexcept {
    if (a != b) {
        expect(false, loc);
        log() << a << " !=\n" << b << "\n\n";
    }
}

template<typename T, typename U>
void expect_eq(T const &a, U const &b, etest::source_location const &loc = etest::source_location::current()) noexcept {
    expect(a == b, loc);
}

// Hard test requirement. Prints the types compared on failure (if printable).
template<Printable T, Printable U>
void require_eq(T const &a, U const &b, etest::source_location const &loc = etest::source_location::current()) {
    if (a != b) {
        try {
            require(false, loc);
        } catch (...) {
            log() << a << " !=\n" << b << "\n\n";
            throw;
        }
    }
}

template<typename T, typename U>
void require_eq(T const &a, U const &b, etest::source_location const &loc = etest::source_location::current()) {
    require(a == b, loc);
}

} // namespace etest

#endif
