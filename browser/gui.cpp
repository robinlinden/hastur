// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "browser/gui/app.h"

#include <spdlog/cfg/env.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <string_view>

using namespace std::literals;

namespace {
auto const kBrowserTitle{"hastur"};
auto const kStartpage{"http://example.com"};
} // namespace

int main(int argc, char **argv) {
    spdlog::set_default_logger(spdlog::stderr_color_mt(kBrowserTitle));
    spdlog::cfg::load_env_levels();

    bool load_page = argc > 1; // Load page right away if provided on the cmdline.
    browser::gui::App app{kBrowserTitle, argc > 1 ? argv[1] : kStartpage, load_page};
    for (int i = 0; i < argc; ++i) {
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

            app.set_scale(maybe_scale[0] - '0');
            break;
        }
    }

    return app.run();
}
