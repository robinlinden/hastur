#include "html/parser.h"
#include "tui/tui.h"

#include <asio.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/cfg/env.h>

#include <cassert>
#include <iostream>
#include <sstream>
#include <string>

using namespace std::string_literals;

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

std::string drop_http_headers(std::string html) {
    const auto delim = "\r\n\r\n"s;
    auto it = html.find(delim);
    html.erase(0, it + delim.size());
    return html;
}

} // namespace

int main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) {
    spdlog::cfg::load_env_levels();
    spdlog::info("Fetching HTML");
    asio::ip::tcp::iostream stream("www.example.com", "http");
    stream << "GET / HTTP/1.1\r\n";
    stream << "Host: www.example.com\r\n";
    stream << "Accept: text/html\r\n";
    stream << "Connection: close\r\n\r\n";
    stream.flush();

    std::stringstream ss;
    ss << stream.rdbuf();
    auto buffer = ss.str();

    buffer = drop_http_headers(buffer);

    spdlog::info("Parsing HTML");
    auto nodes = html::Parser{buffer}.parse_nodes();
    for (auto const &node : nodes) { print_node(node); }

    spdlog::info("Building TUI");
    for (auto const &node : nodes) { std::cout << tui::render(node) << '\n'; }
    spdlog::info("Done");
}
