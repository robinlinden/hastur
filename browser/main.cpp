#include "html/parser.h"
#include "http/get.h"
#include "tui/tui.h"

#include <spdlog/spdlog.h>
#include <spdlog/cfg/env.h>

#include <iostream>

namespace {

template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

// Not needed as of C++20, but gcc 10 won't work without it.
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

void print_node(dom::Node node, uint8_t depth = 0) {
    for (int8_t i = 0; i < depth; ++i) { std::cout << "  "; }
    std::visit(overloaded {
        [](std::monostate) {},
        [](dom::Doctype const &doctype) { std::cout << "doctype: " << doctype.doctype << '\n'; },
        [](dom::Element const &element) { std::cout << "tag: " << element.name << '\n'; },
        [](dom::Text const &text) { std::cout << "value: " << text.text << '\n'; },
    }, node.data);

    for (auto const &child : node.children) { print_node(child, depth + 1); }
}

} // namespace

int main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) {
    spdlog::cfg::load_env_levels();

    const char *endpoint = argc > 1 ? argv[1] : "www.example.com";

    spdlog::info("Fetching HTML from {}", endpoint);
    auto response = http::get(endpoint);

    spdlog::info("Parsing HTML");
    auto nodes = html::Parser{response.body}.parse_nodes();
    for (auto const &node : nodes) { print_node(node); }

    spdlog::info("Building TUI");
    for (auto const &node : nodes) { std::cout << tui::render(node) << '\n'; }

    spdlog::info("Done");
}
