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

namespace {
char const *const kDefaultUri = "http://www.example.com";
} // namespace

int main(int argc, char **argv) {
    spdlog::set_default_logger(spdlog::stderr_color_mt("hastur"));
    spdlog::cfg::load_env_levels();

    auto uri = argc > 1 ? uri::Uri::parse(argv[1]) : uri::Uri::parse(kDefaultUri);
    if (!uri) {
        spdlog::error("Unable to parse uri from {}", argc > 1 ? argv[1] : kDefaultUri);
        return 1;
    }

    engine::Engine engine{protocol::HandlerFactory::create()};
    if (auto err = engine.navigate(*uri); err != protocol::Error::Ok) {
        spdlog::error("Got error {} from {}", static_cast<int>(err), uri->uri);
        std::exit(1);
    }

    std::cout << dom::to_string(engine.dom());
    spdlog::info("Building TUI");
    std::cout << tui::render(engine.layout()) << '\n';
    spdlog::info("Done");
}
