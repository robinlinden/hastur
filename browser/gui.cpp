// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "browser/gui/app.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace {
auto const kBrowserTitle{"hastur"};
auto const kStartpage{"http://example.com"};
} // namespace

int main(int argc, char **argv) {
    spdlog::set_default_logger(spdlog::stderr_color_mt(kBrowserTitle));

    bool load_page = argc > 1; // Load page right away if provided on the cmdline.
    browser::gui::App app{kBrowserTitle, argc > 1 ? argv[1] : kStartpage, load_page};
    return app.run();
}
