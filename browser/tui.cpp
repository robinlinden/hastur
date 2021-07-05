#include "dom/dom.h"
#include "html/parse.h"
#include "http/get.h"
#include "layout/layout.h"
#include "style/style.h"
#include "tui/tui.h"

#include <spdlog/spdlog.h>
#include <spdlog/cfg/env.h>

#include <iostream>

namespace {
char const *const kDefaultUri = "http://www.example.com";
} // namespace

int main(int argc, char **argv) {
    spdlog::cfg::load_env_levels();

    auto uri = argc > 1 ? util::Uri::parse(argv[1]) : util::Uri::parse(kDefaultUri);
    if (!uri) {
        spdlog::error("Unable to parse uri from {}", argc > 1 ? argv[1] : kDefaultUri);
        return 1;
    }

    spdlog::info("Fetching HTML from {}", uri->uri);
    auto response = http::get(*uri);
    if (response.err != http::Error::Ok) {
        spdlog::error("Got error {} from {}", response.err, uri->uri);
        return 1;
    }

    spdlog::info("Parsing HTML");
    auto dom = html::parse(response.body);
    std::cout << dom::to_string(dom);

    spdlog::info("Styling tree");
    std::vector<css::Rule> stylesheet{{{"head"}, {{"display", "none"}}}};
    auto styled = style::style_tree(dom.html, stylesheet);

    spdlog::info("Creating layout");
    // 0 as the width is fine as we don't use the measurements when rendering the tui.
    auto layout = layout::create_layout(styled, 0);

    spdlog::info("Building TUI");
    std::cout << tui::render(layout) << '\n';

    spdlog::info("Done");
}
