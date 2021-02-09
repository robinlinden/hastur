#include "tui/tui.h"

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/screen/string.hpp>

#include <iostream>
#include <optional>
#include <string>
#include <variant>

namespace tui {
namespace {

template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

// Not needed as of C++20, but gcc 10 won't work without it.
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

std::optional<ftxui::Element> element_from_node(dom::Node const &node) {
    ftxui::Elements children;
    for (auto const &child : node.children) {
        if (auto n = element_from_node(child); n) {
            children.push_back(*n);
        }
    };

    return std::visit(overloaded {
        [](std::monostate)  -> std::optional<ftxui::Element> { return std::nullopt; },
        [&](dom::Doctype const &) -> std::optional<ftxui::Element> { return std::nullopt; },
        [&](dom::Element const &node) -> std::optional<ftxui::Element> {
            if (node.name == "html") { return border(children[0]); }
            else if (node.name == "body") { return vbox(children); }
            else if (node.name == "div") { return flex(vbox(children)); }
            else if (node.name == "h1") { return underlined(vbox(children)); }
            else if (node.name == "p") { return flex(vbox(children)); }
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

} // namespace

using namespace std::string_literals;

std::string render(dom::Node const &root) {
    auto document = element_from_node(root);
    if (!document) { return ""s; }
    document = *document | ftxui::size(ftxui::WIDTH, ftxui::LESS_THAN, 80);
    auto screen = ftxui::Screen::Create(ftxui::Dimension{80, 10});
    ftxui::Render(screen, *document);
    return screen.ToString();
}

} // namespace tui
