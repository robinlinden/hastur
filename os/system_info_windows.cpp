// SPDX-FileCopyrightText: 2022-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "os/system_info.h"

#include "os/windows_setup.h" // IWYU pragma: keep

#include <shellscalingapi.h>

#include <array>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>

namespace os {
// NOLINTBEGIN(concurrency-mt-unsafe): We never modify the environment variables.

unsigned active_window_scale_factor() {
    if (auto const *env_var = std::getenv("HST_SCALE")) {
        if (int result{}; std::from_chars(env_var, env_var + std::strlen(env_var), result).ec == std::errc{}) {
            return result;
        }
    }

    DEVICE_SCALE_FACTOR scale_factor{};
    if (GetScaleFactorForMonitor(MonitorFromWindow(GetActiveWindow(), MONITOR_DEFAULTTONEAREST), &scale_factor)
            != S_OK) {
        return 1;
    }

    return static_cast<unsigned>(std::lround(static_cast<float>(scale_factor) / 100.f));
}

bool is_dark_mode() {
    if (auto const *env_var = std::getenv("HST_DARK_MODE")) {
        return std::strcmp(env_var, "1") == 0;
    }

    std::array<std::uint8_t, sizeof(DWORD)> value{};
    auto size = static_cast<DWORD>(value.size());
    static constexpr auto *kPath = R"(Software\Microsoft\Windows\CurrentVersion\Themes\Personalize)";
    if (auto ret = RegGetValueA(
                HKEY_CURRENT_USER, kPath, "AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, value.data(), &size);
            ret != ERROR_SUCCESS) {
        std::cerr << "Unable to read system dark mode preference: '" << ret << "'\n";
        return false;
    }

    return value[0] == 0;
}

// NOLINTEND(concurrency-mt-unsafe)
} // namespace os
