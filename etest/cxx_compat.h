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
class source_location {
public:
    static constexpr source_location current(std::uint_least32_t line = __builtin_LINE(),
            std::uint_least32_t column = __builtin_COLUMN(),
            char const *file_name = __builtin_FILE()) noexcept {
        return {line, column, file_name};
    }
    constexpr std::uint_least32_t line() const noexcept { return line_; }
    constexpr std::uint_least32_t column() const noexcept { return column_; }
    constexpr char const *file_name() const noexcept { return file_name_; }
    constexpr char const *function_name() const noexcept { return ""; }

private:
    std::uint_least32_t line_{};
    std::uint_least32_t column_{};
    char const *file_name_{};

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    constexpr source_location(std::uint_least32_t line, std::uint_least32_t column, char const *file_name) noexcept
        : line_{line}, column_(column), file_name_{file_name} {}
};
} // namespace etest

#endif

#endif
