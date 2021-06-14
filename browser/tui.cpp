#include "dom/dom.h"
#include "html/parse.h"
#include "http/get.h"
#include "layout/layout.h"
#include "style/style.h"
#include "tui/tui.h"

#include <spdlog/spdlog.h>
#include <spdlog/cfg/env.h>

#include <iostream>

int main(int argc, char **argv) {
    spdlog::cfg::load_env_levels();

    const char *endpoint = argc > 1 ? argv[1] : "www.example.com";

    spdlog::info("Fetching HTML from {}", endpoint);
    auto response = http::get(endpoint);
    if (response.err != http::Error::Ok) {
        spdlog::error("Got error {} from {}", response.err, endpoint);
        return 1;
    }

    spdlog::info("Parsing HTML");
    auto dom = html::parse(response.body);
    std::cout << dom::to_string(dom);

    spdlog::info("Styling tree");
    std::vector<css::Rule> stylesheet{{{"head"}, {{"display", "none"}}}};
    auto styled = style::style_tree(dom.children[0], stylesheet);

    spdlog::info("Creating layout");
    // 0 as the width is fine as we don't use the measurements when rendering the tui.
    auto layout = layout::create_layout(styled, 0);

    spdlog::info("Building TUI");
    std::cout << tui::render(layout) << '\n';

    spdlog::info("Done");
}
