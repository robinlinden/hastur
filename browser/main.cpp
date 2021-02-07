#include "parser/parser.h"

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/screen/string.hpp>
#include <asio.hpp> // Needs to be after ftxui due to pulling in macros that break their headers.

#include <cassert>
#include <iostream>
#include <optional>
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
        [](dom::Doctype const &node) { std::cout << "doctype: " << node.doctype << '\n'; },
        [](dom::Element const &node) { std::cout << "tag: " << node.name << '\n'; },
        [](dom::Text const &node) { std::cout << "value: " << node.text << '\n'; },
    }, node.data);

    for (auto const &child : node.children) { print_node(child, depth + 1); }
}

std::string drop_http_headers(std::string html) {
    const auto delim = "\r\n\r\n"s;
    auto it = html.find(delim);
    html.erase(0, it + delim.size());
    return html;
}

std::optional<ftxui::Element> create_ftxui_ui(dom::Node const &node) {
    ftxui::Elements children;
    for (auto const &child : node.children) {
        if (auto n = create_ftxui_ui(child); n) {
            children.push_back(*n);
        }
    };

    return std::visit(overloaded {
        [](std::monostate)  -> std::optional<ftxui::Element> { return std::nullopt; },
        [&](dom::Doctype const &) -> std::optional<ftxui::Element> { return std::nullopt; },
        [&](dom::Element const &node) -> std::optional<ftxui::Element> {
            if (node.name == "html") { return border(children[0]); }
            else if (node.name == "body") { return vbox(children); }
            else if (node.name == "div") { return vbox(children); }
            else if (node.name == "h1") { return underlined(vbox(children)); }
            else if (node.name == "p") { return vbox(children); }
            else if (node.name == "a") { return bold(vbox(children)); }
            else {
                std::cout << "Unhandled node: " << node.name << '\n';
                return std::nullopt;
            }
        },
        [&](dom::Text const &node) -> std::optional<ftxui::Element> {
            return hflow(ftxui::paragraph(ftxui::to_wstring(node.text)));
        },
    }, node.data);
}

void ftxui_test(dom::Node root) {
    std::cout << "\nBuilding TUI\n";
    auto document = *create_ftxui_ui(root);
    if (!document) { return; }
    document = document | ftxui::size(ftxui::WIDTH, ftxui::LESS_THAN, 80);
    auto screen = ftxui::Screen::Create(ftxui::Dimension{80, 10});
    ftxui::Render(screen, document);
    std::cout << screen.ToString() << std::endl;
}

} // namespace

int main(int argc, char **argv) {
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

    auto nodes = parser::Parser{buffer}.parse_nodes();
    for (auto const &node : nodes) { print_node(node); }
    for (auto const &node : nodes) { ftxui_test(node); }
}
