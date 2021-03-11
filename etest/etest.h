#ifndef ETEST_TEST_H_
#define ETEST_TEST_H_

#include <functional>
#include <string_view>

namespace etest {

int run_all_tests() noexcept;
int test(std::string_view name, std::function<void()> body) noexcept;

// Weak test requirement. Allows the test to continue even if the check fails.
void expect(bool) noexcept;

// Hard test requirement. Stops the test (using an exception) if the check fails.
void require(bool);

} // namespace etest

#endif
