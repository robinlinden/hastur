// SPDX-FileCopyrightText: 2021-2025 Robin Lindén <dev@robinlinden.eu>
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
#include <ranges>
#include <regex>
#include <source_location>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

#if defined(_MSC_VER) && defined(_HAS_EXCEPTIONS) && _HAS_EXCEPTIONS != 0
// Unsupported* by Microsoft, but _HAS_EXCEPTIONS is 0 in MSVC if exceptions are disabled.
// See: https://github.com/microsoft/STL/issues/202#issuecomment-545235685
#define ETEST_EXCEPTIONS
#elif defined(__EXCEPTIONS)
// __EXCEPTIONS is set in gcc and Clang unless -fno-exceptions is used.
#define ETEST_EXCEPTIONS
#endif

namespace etest {
namespace {
struct TestFailure : public std::exception {};

struct Actions : public IActions {
    [[noreturn]] void requirement_failure(
            std::optional<std::string_view> log_message, std::source_location const &loc) override {
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

    void expectation_failure(
            std::optional<std::string_view> log_message, std::source_location const &loc) noexcept override {
        ++assertion_failures;
        test_log << "  expectation failure at " << loc.file_name() << "(" << loc.line() << ":" << loc.column() << ")\n";

        if (log_message) {
            test_log << *log_message << "\n\n";
        }
    }

    std::stringstream test_log;
    int assertion_failures{0};
};
} // namespace

int Suite::run(RunOptions const &opts) {
    auto pattern = std::regex{opts.test_name_filter.data(), opts.test_name_filter.size()};
    auto test_name_filter = [&](Test const &test) {
        return std::regex_search(test.name, pattern);
    };

    std::vector<Test> tests_to_run;
    std::ranges::copy(tests_ | std::views::filter(test_name_filter), std::back_inserter(tests_to_run));

    std::cout << tests_.size() + disabled_tests_.size() << " test(s) registered";
    if (disabled_tests_.empty()) {
        std::cout << ".\n" << std::flush;
    } else {
        std::cout << ", " << disabled_tests_.size() << " disabled.\n" << std::flush;
        if (opts.run_disabled_tests) {
            std::ranges::copy(disabled_tests_ | std::views::filter(test_name_filter), std::back_inserter(tests_to_run));
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
