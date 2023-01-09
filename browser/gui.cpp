// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "browser/gui/app.h"

#include "os/os.h"

#include <spdlog/cfg/env.h>
#include <spdlog/sinks/dup_filter_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

using namespace std::literals;

namespace {
auto const kBrowserTitle{"hastur"};
auto const kStartpage{"http://example.com"s};
} // namespace

int main(int argc, char **argv) {
    auto dup_filter = std::make_shared<spdlog::sinks::dup_filter_sink_mt>(std::chrono::seconds{10});
    dup_filter->add_sink(std::make_shared<spdlog::sinks::stderr_color_sink_mt>());
    spdlog::set_default_logger(std::make_shared<spdlog::logger>(kBrowserTitle, std::move(dup_filter)));
    spdlog::cfg::load_env_levels();
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%L%$] %v");

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
