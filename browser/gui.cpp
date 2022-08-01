// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "browser/gui/app.h"

#include "os/os.h"

#include <spdlog/cfg/env.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <optional>
#include <string>
#include <string_view>

using namespace std::literals;

namespace {
auto const kBrowserTitle{"hastur"};
auto const kStartpage{"http://example.com"s};
} // namespace

int main(int argc, char **argv) {
    spdlog::set_default_logger(spdlog::stderr_color_mt(kBrowserTitle));
    spdlog::cfg::load_env_levels();

    std::optional<std::string> page_provided{std::nullopt};
    std::optional<unsigned> scale{std::nullopt};
    for (int i = 1; i < argc; ++i) {
        auto arg = std::string_view{argv[i]};

        if (arg == "--scale"sv) {
            if (i == argc - 1) {
                spdlog::error("Missing argument to --scale");
                return 1;
            }

            auto maybe_scale = std::string_view{argv[i + 1]};
            if (maybe_scale.length() != 1 || maybe_scale[0] < '1' || maybe_scale[0] > '9') {
                spdlog::error("Invalid argument to --scale");
                return 1;
            }

            scale = maybe_scale[0] - '0';
            i += 1;
            continue;
        }

        if (i == argc - 1) {
            page_provided = std::string{arg};
            break;
        }

        spdlog::error("Unhandled arg {} at position {}", arg, i);
        return 1;
    }

    browser::gui::App app{kBrowserTitle, page_provided.value_or(kStartpage), page_provided.has_value()};
    app.set_scale(scale.value_or(os::active_window_scale_factor()));
    return app.run();
}
