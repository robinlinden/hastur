// SPDX-FileCopyrightText: 2021-2025 Robin Lindén <dev@robinlinden.eu>
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
concept Ostreamable = requires(std::ostream &os, T t) {
    { os << t } -> std::same_as<std::ostream &>;
};

template<typename T>
concept HasToString = requires(T t) {
    { to_string(t) } -> std::convertible_to<std::string_view>;
};

template<typename T>
concept Printable = Ostreamable<T> || HasToString<T>;

template<typename T, typename U>
void print_to(std::ostream &, std::string_view, T const &, U const &) {}

template<Printable T, Printable U>
void print_to(std::ostream &os, std::string_view actual_op, T const &a, U const &b) {
    if constexpr (Ostreamable<T>) {
        os << a;
    } else {
        os << to_string(a);
    }

    os << ' ' << actual_op << '\n';

    if constexpr (Ostreamable<U>) {
        os << b;
    } else {
        os << to_string(b);
    }
}

struct RunOptions {
    bool run_disabled_tests{false};
    bool enable_color_output{true};
    std::optional<unsigned> rng_seed;
    // Pattern to match test names against. Must be a valid regex compatible w/ std::regex.
    std::string_view test_name_filter{".*"};
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

        expectation_failure(log_message, loc);
    }

    // Hard test requirement. Stops the test (using an exception) if the check fails.
    void require(bool requirement,
            std::optional<std::string_view> log_message = std::nullopt,
            std::source_location const &loc = std::source_location::current()) {
        if (requirement) {
            return;
        }

        requirement_failure(log_message, loc);
    }

    // Weak test requirement. Prints the types compared on failure (if printable).
    constexpr void expect_eq(auto const &a,
            auto const &b,
            std::optional<std::string_view> log_message = std::nullopt,
            std::source_location const &loc = std::source_location::current()) noexcept {
        if (a == b) {
            return;
        }

        std::stringstream ss;
        print_to(ss, "!=", a, b);
        expect(false, log_message || ss.view().empty() ? log_message : std::move(ss).str(), loc);
    }

    // Hard test requirement. Prints the types compared on failure (if printable).
    constexpr void require_eq(auto const &a,
            auto const &b,
            std::optional<std::string_view> log_message = std::nullopt,
            std::source_location const &loc = std::source_location::current()) {
        if (a == b) {
            return;
        }

        std::stringstream ss;
        print_to(ss, "!=", a, b);
        require(false, log_message || ss.view().empty() ? log_message : std::move(ss).str(), loc);
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
    std::optional<std::string> name_;
    std::vector<Test> tests_;
    std::vector<Test> disabled_tests_;
};

} // namespace etest

#endif
