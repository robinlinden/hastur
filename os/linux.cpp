// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "os/os.h"

#include <sys/mman.h>

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

ExecutableMemory::~ExecutableMemory() {
    if (memory_ != nullptr && munmap(memory_, size_) != 0) {
        std::abort();
    }
}

std::optional<ExecutableMemory> ExecutableMemory::allocate_containing(std::span<std::uint8_t const> data) {
    if (data.empty()) {
        return std::nullopt;
    }

    auto *memory = mmap(nullptr, data.size(), PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (memory == MAP_FAILED) {
        return std::nullopt;
    }

    std::memcpy(memory, data.data(), data.size());

    if (mprotect(memory, data.size(), PROT_EXEC) != 0) {
        if (munmap(memory, data.size()) != 0) {
            std::abort();
        }

        return std::nullopt;
    }

    return ExecutableMemory{memory, data.size()};
}

} // namespace os
