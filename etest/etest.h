#ifndef ETEST_TEST_H_
#define ETEST_TEST_H_

#include <functional>
#include <string_view>

namespace etest {

int run_all_tests() noexcept;
int test(std::string_view name, std::function<void()> body) noexcept;
void expect_true(bool) noexcept;

} // namespace etest

#endif
