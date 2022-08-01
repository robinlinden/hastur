// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

// https://man7.org/linux/man-pages/man3/setenv.3.html
#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif
#define _POSIX_C_SOURCE 200112L

#include "os/os.h"

#include "etest/etest.h"

#include <stdlib.h>

using etest::expect_eq;

int main() {
    // Ensure that the system's environment doesn't affect the test result.
    unsetenv("QT_SCALE_FACTOR");
    unsetenv("GDK_SCALE");
    unsetenv("ELM_SCALE");

    etest::test("active_window_scale_factor", [] {
        // We default to 1 when no GUI toolkit has an opinion.
        expect_eq(os::active_window_scale_factor(), 1u);

        setenv("ELM_SCALE", "2", false);
        expect_eq(os::active_window_scale_factor(), 2u);

        setenv("GDK_SCALE", "5", false);
        expect_eq(os::active_window_scale_factor(), 5u);

        setenv("QT_SCALE_FACTOR", "10", false);
        expect_eq(os::active_window_scale_factor(), 10u);
    });

    return etest::run_all_tests();
}
