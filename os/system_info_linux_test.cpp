// SPDX-FileCopyrightText: 2022-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

// https://man7.org/linux/man-pages/man3/setenv.3.html
#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif
// NOLINTNEXTLINE: The only option is to mess with this reserved identifier.
#define _POSIX_C_SOURCE 200112L

#include "os/system_info.h"

#include "etest/etest2.h"

// This is the header POSIX says we need to include for setenv/unsetenv.
// NOLINTNEXTLINE
#include <stdlib.h>

// NOLINTBEGIN(concurrency-mt-unsafe): No threads here.

int main() {
    etest::Suite s{"os/linux::system_info"};

    // Ensure that the system's environment doesn't affect the test result.
    unsetenv("HST_SCALE");
    unsetenv("QT_SCALE_FACTOR");
    unsetenv("GDK_SCALE");
    unsetenv("ELM_SCALE");

    unsetenv("HST_DARK_MODE");

    s.add_test("active_window_scale_factor", [](etest::IActions &a) {
        static constexpr int kOnlyIfUnset = 0;

        // We default to 1 when no GUI toolkit has an opinion.
        a.expect_eq(os::active_window_scale_factor(), 1u);

        setenv("ELM_SCALE", "2", kOnlyIfUnset);
        a.expect_eq(os::active_window_scale_factor(), 2u);

        setenv("GDK_SCALE", "5", kOnlyIfUnset);
        a.expect_eq(os::active_window_scale_factor(), 5u);

        setenv("QT_SCALE_FACTOR", "10", kOnlyIfUnset);
        a.expect_eq(os::active_window_scale_factor(), 10u);

        setenv("HST_SCALE", "50", kOnlyIfUnset);
        a.expect_eq(os::active_window_scale_factor(), 50u);
    });

    s.add_test("is_dark_mode", [](etest::IActions &a) {
        static constexpr int kOverwrite = 1;

        // We default to false.
        a.expect_eq(os::is_dark_mode(), false);

        setenv("HST_DARK_MODE", "0", kOverwrite);
        a.expect_eq(os::is_dark_mode(), false);

        setenv("HST_DARK_MODE", "1", kOverwrite);
        a.expect_eq(os::is_dark_mode(), true);
    });

    return s.run();
}

// NOLINTEND(concurrency-mt-unsafe)
