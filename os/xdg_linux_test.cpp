// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

// https://man7.org/linux/man-pages/man3/setenv.3.html
#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif
// NOLINTNEXTLINE: The only option is to mess with this reserved identifier.
#define _POSIX_C_SOURCE 200112L

#include "os/xdg.h"

#include "etest/etest2.h"

// This is the header POSIX says we need to include for setenv/unsetenv.
// NOLINTNEXTLINE
#include <stdlib.h>

#include <algorithm>

// NOLINTBEGIN(concurrency-mt-unsafe): No threads here.

int main() {
    static constexpr int kOnlyIfUnset = 0;

    // Ensure that the system's environment doesn't affect the test result.
    unsetenv("HOME");
    unsetenv("XDG_DATA_HOME");

    etest::Suite s{"os::xdg/linux"};

    auto const font_paths_without_env_vars = os::font_paths();

    s.add_test("HOME", [&](etest::IActions &a) {
        static constexpr auto kHome = "/home";
        setenv("HOME", kHome, kOnlyIfUnset);

        a.expect(std::ranges::find_if(font_paths_without_env_vars, [](auto const &path) {
            return path.contains(kHome);
        }) == font_paths_without_env_vars.end());

        auto font_paths = os::font_paths();
        a.expect(std::ranges::find_if(font_paths, [](auto const &path) { return path.contains(kHome); })
                != font_paths.end());
        unsetenv("HOME");
    });

    s.add_test("XDG_DATA_HOME", [&](etest::IActions &a) {
        static constexpr auto kXdgDataHome = "/xdg_data_home";
        setenv("XDG_DATA_HOME", kXdgDataHome, kOnlyIfUnset);

        a.expect(std::ranges::find_if(font_paths_without_env_vars, [](auto const &path) {
            return path.contains(kXdgDataHome);
        }) == font_paths_without_env_vars.end());

        auto font_paths = os::font_paths();
        a.expect(std::ranges::find_if(font_paths, [](auto const &path) { return path.contains(kXdgDataHome); })
                != font_paths.end());
        unsetenv("XDG_DATA_HOME");
    });

    return s.run();
}

// NOLINTEND(concurrency-mt-unsafe)
