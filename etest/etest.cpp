// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "etest/etest.h"

#include <algorithm>
#include <exception>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

namespace etest {
namespace {

int assertion_failures = 0;

struct Test {
    std::string_view name;
    std::function<void()> body;
};

std::vector<Test> &registry() {
    static std::vector<Test> test_registry;
    return test_registry;
}

struct test_failure : public std::exception {};

std::stringstream test_log{};

} // namespace

int run_all_tests() noexcept {
    std::cout << registry().size() << " test(s) registered." << std::endl;
    auto const longest_name = std::max_element(registry().begin(), registry().end(), [](auto const &a, auto const &b) {
        return a.name.size() < b.name.size();
    });

    for (auto const &test : registry()) {
        std::cout << std::left << std::setw(longest_name->name.size()) << test.name << ": ";
        test_log = std::stringstream{};

        const int before = assertion_failures;

        try {
            test.body();
        } catch (...) {
            ++assertion_failures;
        }

        if (before == assertion_failures) {
            std::cout << "\u001b[32mPASSED\u001b[0m\n";
        } else {
            std::cout << "\u001b[31;1mFAILED\u001b[0m\n";
            std::cout << test_log.str();
        }
    }

    return assertion_failures > 0 ? 1 : 0;
}

int test(std::string_view name, std::function<void()> body) noexcept {
    registry().push_back({name, body});
    return 0;
}

void expect(bool b, etest::source_location const &loc) noexcept {
    if (!b) {
        ++assertion_failures;
        // Check if we're using the real source_location by checking for line == 0.
        if (loc.line() != 0) {
            test_log << "  expectation failure at " << loc.file_name() << "(" << loc.line() << ":" << loc.column()
                     << ")\n";
        }
    }
}

void require(bool b, etest::source_location const &loc) {
    if (!b) {
        if (loc.line() != 0) {
            test_log << "  requirement failure at " << loc.file_name() << "(" << loc.line() << ":" << loc.column()
                     << ")\n";
        }
        throw test_failure{};
    }
}

std::ostream &log() {
    return test_log;
}

} // namespace etest
