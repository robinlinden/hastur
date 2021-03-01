#include "etest/etest.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
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

} // namespace

int run_all_tests() noexcept {
    std::cout << registry().size() << " test(s) registered." << std::endl;
    auto const longest_name = std::max_element(registry().begin(), registry().end(),
            [](auto const &a, auto const &b) { return a.name.size() < b.name.size(); });

    for (auto const &test : registry()) {
        std::cout << std::left << std::setw(longest_name->name.size()) << test.name << ": ";

        const int before = assertion_failures;

        try {
            test.body();
        } catch (...) {
            ++assertion_failures;
        }

        std::cout << (before == assertion_failures ?
                "\u001b[32mPASSED\u001b[0m\n" : "\u001b[31;1mFAILED\u001b[0m\n");
    }

    return assertion_failures > 0 ? 1 : 0;
}

int test(std::string_view name, std::function<void()> body) noexcept {
    registry().push_back({name, body});
    return 0;
}

void expect_true(bool b) noexcept {
    if (!b) { ++assertion_failures; }
}

} // namespace etest
