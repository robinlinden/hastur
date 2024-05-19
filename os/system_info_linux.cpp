// SPDX-FileCopyrightText: 2021-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "os/system_info.h"

#include <array>
#include <charconv>
#include <cstdlib>
#include <cstring>
#include <system_error>

namespace os {

// This is okay as long as we don't call e.g. setenv(), unsetenv(), or putenv().
// NOLINTBEGIN(concurrency-mt-unsafe)

unsigned active_window_scale_factor() {
    // Hastur, Qt, Gnome, and Elementary in that order.
    // Environment variables from https://wiki.archlinux.org/title/HiDPI#GUI_toolkits
    for (auto const *env_var : std::array{"HST_SCALE", "QT_SCALE_FACTOR", "GDK_SCALE", "ELM_SCALE"}) {
        if (char const *scale = std::getenv(env_var)) {
            int result{};
            if (std::from_chars(scale, scale + std::strlen(scale), result).ec == std::errc{}) {
                return result;
            }
        }
    }

    return 1;
}

bool is_dark_mode() {
    if (auto const *env_var = std::getenv("HST_DARK_MODE")) {
        return std::strcmp(env_var, "1") == 0;
    }

    return false;
}

// NOLINTEND(concurrency-mt-unsafe)

} // namespace os
