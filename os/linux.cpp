// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "os/os.h"

#include <array>
#include <charconv>
#include <cstdlib>
#include <cstring>

using namespace std::literals;

namespace os {

// This is okay as long as we don't call e.g. setenv(), unsetenv(), or putenv().
// NOLINTBEGIN(concurrency-mt-unsafe)

std::vector<std::string> font_paths() {
    std::vector<std::string> paths{};
    if (char const *xdg_data_home = std::getenv("XDG_DATA_HOME")) {
        paths.push_back(xdg_data_home + "/fonts"s);
    }

    if (char const *home = std::getenv("HOME")) {
        if (paths.empty()) {
            paths.push_back(home + "/.local/share/fonts"s);
        }
        paths.push_back(home + "/.fonts"s);
    }

    paths.push_back("/usr/share/fonts"s);
    paths.push_back("/usr/local/share/fonts"s);
    return paths;
}

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

// NOLINTEND(concurrency-mt-unsafe)

} // namespace os
