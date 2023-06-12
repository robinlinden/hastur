// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "etest/etest.h"

#include <algorithm>
#include <cstdlib>
#include <exception>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <ostream>
#include <sstream>
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

int assertion_failures = 0;

struct Test {
    std::string name;
    std::function<void()> body;
};

struct Registry {
    std::vector<Test> tests;
    std::vector<Test> disabled_tests;
};

Registry &registry() {
    static Registry test_registry;
    return test_registry;
}

struct TestFailure : public std::exception {};

std::stringstream test_log{};

} // namespace

int run_all_tests(RunOptions const &opts) noexcept {
    auto const &tests = registry().tests;
    auto const &disabled = registry().disabled_tests;

    std::vector<Test> tests_to_run;
    std::ranges::copy(begin(tests), end(tests), std::back_inserter(tests_to_run));

    std::cout << tests.size() + disabled.size() << " test(s) registered";
    if (disabled.size() == 0) {
        std::cout << "." << std::endl;
    } else {
        std::cout << ", " << disabled.size() << " disabled." << std::endl;
        if (opts.run_disabled_tests) {
            std::ranges::copy(begin(disabled), end(disabled), std::back_inserter(tests_to_run));
        }
    }

    if (tests_to_run.empty()) {
        return 1;
    }

    // TODO(robinlinden): std::ranges once clang-cl supports it. Last tested
    // with LLVM 15.0.0.
    auto const longest_name = std::max_element(tests_to_run.begin(),
            tests_to_run.end(),
            [](auto const &a, auto const &b) { return a.name.size() < b.name.size(); });

    for (auto const &test : tests_to_run) {
        std::cout << std::left << std::setw(longest_name->name.size()) << test.name << ": ";

        int const before = assertion_failures;

#ifdef ETEST_EXCEPTIONS
        try {
            test.body();
        } catch (TestFailure const &) {
            ++assertion_failures;
        } catch (std::exception const &e) {
            ++assertion_failures;
            test_log << "Unhandled exception in test body: " << e.what() << '\n';
        } catch (...) {
            ++assertion_failures;
            test_log << "Unhandled unknown exception in test body.\n";
        }
#else
        test.body();
#endif

        if (before == assertion_failures) {
            std::cout << "\u001b[32mPASSED\u001b[0m\n";
        } else {
            std::cout << "\u001b[31;1mFAILED\u001b[0m\n";
            std::cout << std::exchange(test_log, {}).str();
        }

        std::cout << std::flush;
    }

    return assertion_failures > 0 ? 1 : 0;
}

void test(std::string name, std::function<void()> body) noexcept {
    // TODO(robinlinden): push_back -> emplace_back once Clang supports it (C++20/p0960). Not supported as of Clang 13.
    registry().tests.push_back({std::move(name), std::move(body)});
}

void disabled_test(std::string name, std::function<void()> body) noexcept {
    registry().disabled_tests.push_back({std::move(name), std::move(body)});
}

void expect(bool b, std::optional<std::string_view> log_message, etest::source_location const &loc) noexcept {
    if (b) {
        return;
    }

    ++assertion_failures;
    // Check if we're using the real source_location by checking for line == 0.
    if (loc.line() != 0) {
        test_log << "  expectation failure at " << loc.file_name() << "(" << loc.line() << ":" << loc.column() << ")\n";
    }

    if (log_message) {
        test_log << *log_message << "\n\n";
    }
}

void require(bool b, std::optional<std::string_view> log_message, etest::source_location const &loc) {
    if (b) {
        return;
    }

    if (loc.line() != 0) {
        test_log << "  requirement failure at " << loc.file_name() << "(" << loc.line() << ":" << loc.column() << ")\n";
    }

    if (log_message) {
        test_log << *log_message << "\n\n";
    }

#ifdef ETEST_EXCEPTIONS
    throw TestFailure{};
#else
    std::abort();
#endif
}

} // namespace etest
