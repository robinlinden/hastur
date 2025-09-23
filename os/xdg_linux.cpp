// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "os/xdg.h"

#include <cstdlib>
#include <string>
#include <vector>

using namespace std::literals;

namespace os {

// This is okay as long as we don't call e.g. setenv(), unsetenv(), or putenv().
// NOLINTBEGIN(concurrency-mt-unsafe)

std::vector<std::string> font_paths() {
    std::vector<std::string> paths;

    char const *home = std::getenv("HOME");
    if (char const *xdg_data_home = std::getenv("XDG_DATA_HOME")) {
        paths.push_back(xdg_data_home + "/fonts"s);
    } else if (home != nullptr) {
        // $HOME/.local/share/ is the default XDG_DATA_HOME, so we only add this
        // path if XDG_DATA_HOME is not set.
        paths.push_back(home + "/.local/share/fonts"s);
    }

    if (home != nullptr) {
        paths.push_back(home + "/.fonts"s);
    }

    paths.push_back("/usr/share/fonts"s);
    paths.push_back("/usr/local/share/fonts"s);
    return paths;
}

// NOLINTEND(concurrency-mt-unsafe)

} // namespace os
