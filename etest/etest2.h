// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef ETEST_ETEST2_H_
#define ETEST_ETEST2_H_

#include "etest/cxx_compat.h"

#include <functional>
#include <iosfwd>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace etest {
// NOLINTBEGIN(google-default-arguments): Some things like log messages and
// source locations are optional.

template<typename T>
concept Printable = requires(std::ostream &os, T t) {
    { os << t } -> std::same_as<std::ostream &>;
};

struct RunOptions {
    bool run_disabled_tests{false};
};

class IActions {
public:
    virtual ~IActions() = default;

    // Weak test requirement. Allows the test to continue even if the check fails.
    virtual void expect(bool,
            std::optional<std::string_view> log_message = std::nullopt,
            etest::source_location const &loc = etest::source_location::current()) noexcept = 0;

    // Hard test requirement. Stops the test (using an exception) if the check fails.
    virtual void require(bool,
            std::optional<std::string_view> log_message = std::nullopt,
            etest::source_location const &loc = etest::source_location::current()) = 0;

    // Weak test requirement. Prints the types compared on failure (if printable).
    template<Printable T, Printable U>
    void expect_eq(T const &a,
            U const &b,
            std::optional<std::string_view> log_message = std::nullopt,
            etest::source_location const &loc = etest::source_location::current()) noexcept {
        if (a == b) {
            return;
        }

        std::stringstream ss;
        ss << a << " !=\n" << b;
        expect(false, log_message ? std::move(log_message) : std::move(ss).str(), loc);
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
        if (a == b) {
            return;
        }

        std::stringstream ss;
        ss << a << " !=\n" << b;
        require(false, log_message ? std::move(log_message) : std::move(ss).str(), loc);
    }

    template<typename T, typename U>
    void require_eq(T const &a,
            U const &b,
            std::optional<std::string_view> log_message = std::nullopt,
            etest::source_location const &loc = etest::source_location::current()) {
        require(a == b, std::move(log_message), loc);
    }
};

struct Test {
    std::string name;
    std::function<void(IActions &)> body;
};

class Suite {
public:
    explicit Suite(std::optional<std::string> name = std::nullopt) : name_(std::move(name)) {}

    void add_test(std::string name, std::function<void(IActions &)> test) {
        tests_.push_back({std::move(name), std::move(test)});
    }

    void disabled_test(std::string name, std::function<void(IActions &)> test) {
        disabled_tests_.push_back({std::move(name), std::move(test)});
    }

    [[nodiscard]] int run(RunOptions const & = {});

private:
    std::optional<std::string> name_{};
    std::vector<Test> tests_{};
    std::vector<Test> disabled_tests_{};
};

// NOLINTEND(google-default-arguments)
} // namespace etest

#endif
