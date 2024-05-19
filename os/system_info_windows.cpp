// SPDX-FileCopyrightText: 2022-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "os/system_info.h"

#include "os/windows_setup.h" // IWYU pragma: keep

#include <shellscalingapi.h>

#include <charconv>
#include <cmath>
#include <cstring>

namespace os {

unsigned active_window_scale_factor() {
    // NOLINTNEXTLINE(concurrency-mt-unsafe): We never modify the environment variables.
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

} // namespace os
