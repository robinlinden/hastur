// SPDX-FileCopyrightText: 2021-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "browser/gui/app.h"

#include "os/system_info.h"
#include "util/arg_parser.h"

#include <spdlog/cfg/env.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/dup_filter_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

using namespace std::literals;

namespace {
constexpr auto kBrowserTitle{"hastur"};
constexpr auto kStartpage{"http://example.com"sv};
} // namespace

int main(int argc, char **argv) {
    auto dup_filter = std::make_shared<spdlog::sinks::dup_filter_sink_mt>(std::chrono::seconds{10});
    dup_filter->add_sink(std::make_shared<spdlog::sinks::stderr_color_sink_mt>());
    spdlog::set_default_logger(std::make_shared<spdlog::logger>(kBrowserTitle, std::move(dup_filter)));
    spdlog::cfg::load_env_levels();
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%L%$] %v");

    std::string page;
    unsigned scale{os::active_window_scale_factor()};
    bool exit_after_load{false};

    auto res = util::ArgParser{}
                       .argument("--scale", scale)
                       .argument("--exit-after-load", exit_after_load)
                       .positional(page)
                       .parse(argc, argv);
    if (!res.has_value()) {
        spdlog::error(res.error().message);
        return 1;
    }

    if (scale < 1 || scale > 9) {
        spdlog::error("Invalid argument to --scale", scale);
        return 1;
    }

    bool const page_provided = !page.empty();
    if (!page_provided) {
        page = kStartpage;
    }

    browser::gui::App app{kBrowserTitle, std::move(page), page_provided};
    app.set_scale(scale);

    if (!exit_after_load) {
        return app.run();
    }

    app.step();
    spdlog::info("Page loaded, exiting...");
    return 0;
}
