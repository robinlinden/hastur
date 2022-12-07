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

std::vector<std::string> font_paths() {
    std::vector<std::string> paths{};
    // This is actually probably a bit user hostile. User fonts *are* better than system fonts
    // but until we can search through for missing glyphs ¯\_(ツ)_/¯
    paths.push_back("/usr/local/share/fonts"s);
    paths.push_back("/usr/share/fonts"s);
    if (char const *xdg_data_home = std::getenv("XDG_DATA_HOME")) {
        paths.push_back(xdg_data_home + "/fonts"s);
    }

    if (char const *home = std::getenv("HOME")) {
        if (paths.empty()) {
            paths.push_back(home + "/.local/share/fonts"s);
        }
        paths.push_back(home + "/.fonts"s);
    }

    return paths;
}

unsigned active_window_scale_factor() {
    // Hastur, Qt, Gnome, and Elementary in that order.
    // Environment variables from https://wiki.archlinux.org/title/HiDPI#GUI_toolkits
    for (auto *env_var : std::array{"HST_SCALE", "QT_SCALE_FACTOR", "GDK_SCALE", "ELM_SCALE"}) {
        if (char const *scale = std::getenv(env_var)) {
            int result{};
            if (std::from_chars(scale, scale + std::strlen(scale), result).ec == std::errc{}) {
                return result;
            }
        }
    }

    return 1;
}

} // namespace os
