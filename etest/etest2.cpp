// SPDX-FileCopyrightText: 2021-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "etest/etest2.h"

#include <algorithm>
#include <exception>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <optional>
#include <random>
#include <source_location>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

#if defined(_MSC_VER)
// MSVC doesn't seem to have a way of disabling exceptions.
#define ETEST_EXCEPTIONS
#elif defined(__EXCEPTIONS)
// __EXCEPTIONS is set in gcc and Clang unless -fno-exceptions is used.
#define ETEST_EXCEPTIONS
#endif

namespace etest {
namespace {
struct TestFailure : public std::exception {};

struct Actions : public IActions {
    // Weak test requirement. Allows the test to continue even if the check fails.
    void expect(
            bool b, std::optional<std::string_view> log_message, std::source_location const &loc) noexcept override {
        if (b) {
            return;
        }

        ++assertion_failures;
        test_log << "  expectation failure at " << loc.file_name() << "(" << loc.line() << ":" << loc.column() << ")\n";

        if (log_message) {
            test_log << *log_message << "\n\n";
        }
    }

    // Hard test requirement. Stops the test (using an exception) if the check fails.
    void require(bool b, std::optional<std::string_view> log_message, std::source_location const &loc) override {
        if (b) {
            return;
        }

        test_log << "  requirement failure at " << loc.file_name() << "(" << loc.line() << ":" << loc.column() << ")\n";

        if (log_message) {
            test_log << *log_message << "\n\n";
        }

#ifdef ETEST_EXCEPTIONS
        throw TestFailure{};
#else
        std::abort();
#endif
    }

    std::stringstream test_log;
    int assertion_failures{0};
};
} // namespace

int Suite::run(RunOptions const &opts) {
    std::vector<Test> tests_to_run;
    std::ranges::copy(tests_, std::back_inserter(tests_to_run));

    std::cout << tests_.size() + disabled_tests_.size() << " test(s) registered";
    if (disabled_tests_.empty()) {
        std::cout << ".\n" << std::flush;
    } else {
        std::cout << ", " << disabled_tests_.size() << " disabled.\n" << std::flush;
        if (opts.run_disabled_tests) {
            std::ranges::copy(disabled_tests_, std::back_inserter(tests_to_run));
        }
    }

    if (tests_to_run.empty()) {
        return 1;
    }

    unsigned const seed = [&] {
        if (opts.rng_seed) {
            return *opts.rng_seed;
        }

        return std::random_device{}();
    }();
    std::mt19937 rng{seed};

    // Shuffle tests to avoid dependencies between them.
    std::ranges::shuffle(tests_to_run, rng);

    std::cout << "Running " << tests_to_run.size() << " tests with the seed " << seed << ".\n";

    auto const longest_name = std::ranges::max_element(
            tests_to_run, [](auto const &a, auto const &b) { return a.size() < b.size(); }, &Test::name);

    std::vector<Test const *> failed_tests;
    for (auto const &test : tests_to_run) {
        std::cout << std::left << std::setw(longest_name->name.size()) << test.name << ": " << std::flush;

        Actions a{};
#ifdef ETEST_EXCEPTIONS
        try {
            test.body(a);
        } catch (TestFailure const &) {
            a.assertion_failures += 1;
        } catch (std::exception const &e) {
            a.assertion_failures += 1;
            a.test_log << "Unhandled exception in test body: " << e.what() << '\n';
        } catch (...) {
            a.assertion_failures += 1;
            a.test_log << "Unhandled unknown exception in test body.\n";
        }
#else
        test.body(a);
#endif

        if (a.assertion_failures == 0) {
            std::cout << "\u001b[32mPASSED\u001b[0m\n";
        } else {
            std::cout << "\u001b[31;1mFAILED\u001b[0m\n";
            std::cout << std::move(a.test_log).str();
            failed_tests.push_back(&test);
        }

        std::cout << std::flush;
    }

    if (!failed_tests.empty()) {
        std::cout << '\n' << tests_to_run.size() - failed_tests.size() << " passing test(s)\n";
        std::cout << "\u001b[31;1m" << failed_tests.size() << " failing test(s):\u001b[0m\n";
        for (auto const *test : failed_tests) {
            std::cout << "  " << test->name << '\n';
        }
    }

    return failed_tests.empty() ? 0 : 1;
}

} // namespace etest
