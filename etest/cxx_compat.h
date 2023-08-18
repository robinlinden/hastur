// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef ETEST_CXX_COMPAT_H_
#define ETEST_CXX_COMPAT_H_

#include <version>

// Clang 15 has the feature, but reports the wrong file and row, making it less than useful.
#if defined(__cpp_lib_source_location) && (__cpp_lib_source_location >= 201907L) \
        && !(defined(__clang_major__) && (__clang_major__ == 15))

#include <source_location>
namespace etest {
using source_location = std::source_location;
} // namespace etest

#else

#include <cstdint>
namespace etest {
// https://en.cppreference.com/w/cpp/utility/source_location
// NOLINTNEXTLINE(readability-identifier-naming)
struct source_location {
    static constexpr source_location current() noexcept { return source_location{}; }
    constexpr std::uint_least32_t line() const noexcept { return 0; }
    constexpr std::uint_least32_t column() const noexcept { return 0; }
    constexpr char const *file_name() const noexcept { return ""; }
    constexpr char const *function_name() const noexcept { return ""; }
};
} // namespace etest

#endif

#endif
