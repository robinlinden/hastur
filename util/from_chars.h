// SPDX-FileCopyrightText: 2022-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef UTIL_FROM_CHARS_H_
#define UTIL_FROM_CHARS_H_

#include <version> // IWYU pragma: keep

// Workaround for libc++ not supporting std::from_chars for floating point numbers.
// TODO(robinlinden): Nuke once libc++ supports std::from_chars w/ floats.
#if defined(_LIBCPP_VERSION)

#include <cerrno>
#include <charconv>
#include <concepts>
#include <cstdlib>
#include <iterator>
#include <string>
#include <system_error>

namespace util {

// https://en.cppreference.com/w/cpp/utility/from_chars
using std::from_chars;
using std::from_chars_result;

// Not spec-compliant at all, but good enough for how we're using it.
template<std::floating_point T>
inline from_chars_result from_chars(char const *first, char const *last, T &value) {
    // Produce a null-terminated string that we can safely pass to std::strtof.
    std::string to_parse{first, last};
    char *end{};
    T result{};

    errno = 0;
    if constexpr (std::same_as<T, float>) {
        result = std::strtof(to_parse.c_str(), &end);
    } else {
        result = std::strtod(to_parse.c_str(), &end);
    }

    if (end == to_parse.c_str()) {
        // No conversion could be performed.
        return {first, std::errc::invalid_argument};
    }

    // Since we're parsing a copy of the argument, we need to map the end ptr
    // back into the string the user provided.
    auto map_end_into_argument_string = [&] {
        auto parsed_length = std::distance(to_parse.c_str(), static_cast<char const *>(end));
        auto const *new_end = first;
        std::advance(new_end, parsed_length);
        return new_end;
    };

    if (errno == ERANGE) {
        return {map_end_into_argument_string(), std::errc::result_out_of_range};
    }

    value = result;
    return {map_end_into_argument_string(), std::errc{}};
}

} // namespace util

#else

#include <charconv>
namespace util {
using std::from_chars;
using std::from_chars_result;
} // namespace util

#endif

#endif
