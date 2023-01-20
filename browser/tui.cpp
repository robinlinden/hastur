// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "tui/tui.h"
#include "dom/dom.h"
#include "engine/engine.h"
#include "protocol/handler_factory.h"

#include <spdlog/cfg/env.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>

using namespace std::literals;

namespace {
char const *const kDefaultUri = "http://www.example.com";
} // namespace

int main(int argc, char **argv) {
    spdlog::set_default_logger(spdlog::stderr_color_mt("hastur"));
    spdlog::cfg::load_env_levels();
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%L%$] %v");

    auto uri = argc > 1 ? uri::Uri::parse(argv[1]) : uri::Uri::parse(kDefaultUri);
    // Latest Firefox ESR user agent (on Windows). This matches what the Tor browser does.
    auto user_agent = "hastur/0.0 (devel)"s;
    engine::Engine engine{protocol::HandlerFactory::create(std::move(user_agent))};
    if (auto err = engine.navigate(uri); err != protocol::Error::Ok) {
        spdlog::error("Got error {} from {}", static_cast<int>(err), uri.uri);
        std::exit(1);
    }

    std::cout << dom::to_string(engine.dom());
    spdlog::info("Building TUI");
    std::cout << tui::render(engine.layout()) << '\n';
    spdlog::info("Done");
}
