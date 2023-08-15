// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "tui/tui.h"
#include "dom/dom.h"
#include "engine/engine.h"
#include "protocol/handler_factory.h"

#include <spdlog/cfg/env.h>
#include <spdlog/sinks/dup_filter_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

using namespace std::literals;

namespace {
constexpr char const *kDefaultUri = "http://www.example.com";
} // namespace

int main(int argc, char **argv) {
    auto dup_filter = std::make_shared<spdlog::sinks::dup_filter_sink_mt>(std::chrono::seconds{10});
    dup_filter->add_sink(std::make_shared<spdlog::sinks::stderr_color_sink_mt>());
    spdlog::set_default_logger(std::make_shared<spdlog::logger>("hastur", std::move(dup_filter)));
    spdlog::cfg::load_env_levels();
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%L%$] %v");

    auto uri = argc > 1 ? uri::Uri::parse(argv[1]) : uri::Uri::parse(kDefaultUri);
    // Latest Firefox ESR user agent (on Windows). This matches what the Tor browser does.
    auto user_agent = "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:102.0) Gecko/20100101 Firefox/102.0"s;
    engine::Engine engine{protocol::HandlerFactory::create(std::move(user_agent))};
    if (auto err = engine.navigate(uri); err != protocol::Error::Ok) {
        spdlog::error("Got error {} from {}", static_cast<int>(err), uri.uri);
        std::exit(1);
    }

    std::cout << dom::to_string(engine.dom());
    spdlog::info("Building TUI");

    auto const *layout = engine.layout();
    if (layout == nullptr) {
        spdlog::error("Unable to create a layout of {}", uri.uri);
        return 1;
    }

    std::cout << tui::render(*layout) << '\n';
    spdlog::info("Done");
}
