#include "dom/dom.h"
#include "html/parse.h"
#include "http/get.h"
#include "tui/tui.h"

#include <spdlog/spdlog.h>
#include <spdlog/cfg/env.h>

#include <iostream>

int main(int argc, char **argv) {
    spdlog::cfg::load_env_levels();

    const char *endpoint = argc > 1 ? argv[1] : "www.example.com";

    spdlog::info("Fetching HTML from {}", endpoint);
    auto response = http::get(endpoint);

    spdlog::info("Parsing HTML");
    auto nodes = html::parse(response.body);
    for (auto const &node : nodes) { std::cout << dom::to_string(node); }

    spdlog::info("Building TUI");
    for (auto const &node : nodes) { std::cout << tui::render(node) << '\n'; }

    spdlog::info("Done");
}
