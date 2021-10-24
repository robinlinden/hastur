// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "tui/tui.h"
#include "browser/engine.h"
#include "dom/dom.h"

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

    if (uri->path.empty()) {
        uri->path = "/";
    }

    browser::Engine engine;
    engine.set_on_navigation_failure([&](protocol::Error e) {
        spdlog::error("Got error {} from {}", e, uri->uri);
        std::exit(1);
    });

    engine.set_on_page_loaded([&] {
        std::cout << dom::to_string(engine.dom());
        spdlog::info("Building TUI");
        std::cout << tui::render(engine.layout()) << '\n';
    });

    engine.navigate(*uri);

    spdlog::info("Done");
}
