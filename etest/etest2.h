// SPDX-FileCopyrightText: 2021-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef ETEST_ETEST2_H_
#define ETEST_ETEST2_H_

#include <concepts>
#include <exception>
#include <functional>
#include <iosfwd>
#include <optional>
#include <source_location>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace etest {

template<typename T>
concept Printable = requires(std::ostream &os, T t) {
    { os << t } -> std::same_as<std::ostream &>;
};

struct RunOptions {
    bool run_disabled_tests{false};
    std::optional<unsigned> rng_seed;
};

class IActions {
public:
    virtual ~IActions() = default;

    [[noreturn]] virtual void requirement_failure(
            std::optional<std::string_view> log_message, std::source_location const &) = 0;
    virtual void expectation_failure(
            std::optional<std::string_view> log_message, std::source_location const &) noexcept = 0;

    // Weak test requirement. Allows the test to continue even if the check fails.
    void expect(bool expectation,
            std::optional<std::string_view> log_message = std::nullopt,
            std::source_location const &loc = std::source_location::current()) noexcept {
        if (expectation) {
            return;
        }

        expectation_failure(std::move(log_message), loc);
    }

    // Hard test requirement. Stops the test (using an exception) if the check fails.
    void require(bool requirement,
            std::optional<std::string_view> log_message = std::nullopt,
            std::source_location const &loc = std::source_location::current()) {
        if (requirement) {
            return;
        }

        requirement_failure(std::move(log_message), loc);
    }

    // Weak test requirement. Prints the types compared on failure (if printable).
    template<Printable T, Printable U>
    constexpr void expect_eq(T const &a,
            U const &b,
            std::optional<std::string_view> log_message = std::nullopt,
            std::source_location const &loc = std::source_location::current()) noexcept {
        if (a == b) {
            return;
        }

        std::stringstream ss;
        ss << a << " !=\n" << b;
        expect(false, log_message ? std::move(log_message) : std::move(ss).str(), loc);
    }

    template<typename T, typename U>
    constexpr void expect_eq(T const &a,
            U const &b,
            std::optional<std::string_view> log_message = std::nullopt,
            std::source_location const &loc = std::source_location::current()) noexcept {
        expect(a == b, std::move(log_message), loc);
    }

    // Hard test requirement. Prints the types compared on failure (if printable).
    template<Printable T, Printable U>
    constexpr void require_eq(T const &a,
            U const &b,
            std::optional<std::string_view> log_message = std::nullopt,
            std::source_location const &loc = std::source_location::current()) {
        if (a == b) {
            return;
        }

        std::stringstream ss;
        ss << a << " !=\n" << b;
        require(false, log_message ? std::move(log_message) : std::move(ss).str(), loc);
    }

    template<typename T, typename U>
    constexpr void require_eq(T const &a,
            U const &b,
            std::optional<std::string_view> log_message = std::nullopt,
            std::source_location const &loc = std::source_location::current()) {
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

    // TODO(robinlinden): Improve error messages.
    constexpr void constexpr_test(std::string name, auto test) {
        static_assert([test] {
            struct ConstexprActions : IActions {
                void expectation_failure(
                        std::optional<std::string_view>, std::source_location const &) noexcept override {
                    std::terminate();
                }

                void requirement_failure(std::optional<std::string_view>, std::source_location const &) override {
                    std::terminate();
                }
            };
            ConstexprActions a;
            test(a);
            return true;
        }());

        add_test(std::move(name), std::move(test));
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

} // namespace etest

#endif
