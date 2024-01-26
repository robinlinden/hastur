// SPDX-FileCopyrightText: 2024 Robin Lindén <dev@robinlinden.eu>
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
    std::vector<std::string> paths{};

    if (char const *home = std::getenv("HOME"); home != nullptr) {
        paths.push_back(home + "/Library/Fonts"s);
    }

    paths.push_back("/Library/Fonts"s);
    paths.push_back("/System/Library/Fonts"s);
    return paths;
}

// NOLINTEND(concurrency-mt-unsafe)

} // namespace os
