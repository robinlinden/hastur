#ifndef ETEST_TEST_H_
#define ETEST_TEST_H_

// Clang has the header, but it doesn't yet contain std::source_location.
#if __has_include(<source_location>) && !defined(__clang__)
#define ETEST_WITH_SRC_LOC
#endif

#include <functional>
#include <string_view>

#ifdef ETEST_WITH_SRC_LOC
#include <source_location>
#endif

namespace etest {

int run_all_tests() noexcept;
int test(std::string_view name, std::function<void()> body) noexcept;

// Weak test requirement. Allows the test to continue even if the check fails.
#ifndef ETEST_WITH_SRC_LOC
void expect(bool) noexcept;
#else
void expect(bool, std::source_location const &loc = std::source_location::current()) noexcept;
#endif

// Hard test requirement. Stops the test (using an exception) if the check fails.
#ifndef ETEST_WITH_SRC_LOC
void require(bool);
#else
void require(bool, std::source_location const &loc = std::source_location::current());
#endif

} // namespace etest

#endif
