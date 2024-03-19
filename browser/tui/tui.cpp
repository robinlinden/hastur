// SPDX-FileCopyrightText: 2021-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "tui/tui.h"
#include "dom/dom.h"
#include "engine/engine.h"
#include "protocol/handler_factory.h"
#include "protocol/response.h"
#include "uri/uri.h"

#include <fmt/core.h>
#include <spdlog/cfg/env.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/dup_filter_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

using namespace std::literals;

namespace {
constexpr char const *kDefaultUri = "http://www.example.com";

void ensure_has_scheme(std::string &url) {
    if (!url.contains("://")) {
        spdlog::info("Url missing scheme, assuming https");
        url = fmt::format("https://{}", url);
    }
}
} // namespace

int main(int argc, char **argv) {
    auto dup_filter = std::make_shared<spdlog::sinks::dup_filter_sink_mt>(std::chrono::seconds{10});
    dup_filter->add_sink(std::make_shared<spdlog::sinks::stderr_color_sink_mt>());
    spdlog::set_default_logger(std::make_shared<spdlog::logger>("hastur", std::move(dup_filter)));
    spdlog::cfg::load_env_levels();
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%L%$] %v");

    auto uri_str = argc > 1 ? std::string{argv[1]} : kDefaultUri;
    ensure_has_scheme(uri_str);
    auto uri = uri::Uri::parse(uri_str);
    // Latest Firefox ESR user agent (on Windows). This matches what the Tor browser does.
    auto user_agent = "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:102.0) Gecko/20100101 Firefox/102.0"s;
    engine::Engine engine{protocol::HandlerFactory::create(std::move(user_agent))};
    auto maybe_page = engine.navigate(uri);
    if (!maybe_page) {
        spdlog::error(R"(Error loading "{}": {})", uri.uri, to_string(maybe_page.error().response.err));
        return 1;
    }

    auto page = std::move(*maybe_page);

    std::cout << dom::to_string(page->dom);
    spdlog::info("Building TUI");

    if (!page->layout.has_value()) {
        spdlog::error("Unable to create a layout of {}", uri.uri);
        return 1;
    }

    std::cout << tui::render(*page->layout) << '\n';
    spdlog::info("Done");
}
