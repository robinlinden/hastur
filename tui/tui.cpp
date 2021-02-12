#include "tui/tui.h"

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/screen/string.hpp>
#include <spdlog/spdlog.h>

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

std::optional<ftxui::Element> element_from_node(dom::Node const &node);

ftxui::Elements parse_children(dom::Node const &node) {
    ftxui::Elements children;
    for (auto const &child : node.children) {
        if (auto n = element_from_node(child); n) {
            children.push_back(*n);
        }
    };

    return children;
}

std::optional<ftxui::Element> element_from_node(dom::Node const &node) {
    return std::visit(overloaded {
        [](std::monostate)  -> std::optional<ftxui::Element> { return std::nullopt; },
        [&](dom::Doctype const &) -> std::optional<ftxui::Element> { return std::nullopt; },
        [&](dom::Element const &element) -> std::optional<ftxui::Element> {
            if (element.name == "html") { return border(parse_children(node)[0]); }
            else if (element.name == "head") { return std::nullopt; }
            else if (element.name == "body") { return vbox(parse_children(node)); }
            else if (element.name == "div") { return flex(vbox(parse_children(node))); }
            else if (element.name == "h1") { return underlined(vbox(parse_children(node))); }
            else if (element.name == "p") { return flex(vbox(parse_children(node))); }
            else if (element.name == "a") { return bold(vbox(parse_children(node))); }
            else if (element.name == "center") { return hcenter(flex(vbox(parse_children(node)))); }
            else if (element.name == "hr") { return ftxui::separator(); }
            else {
                spdlog::warn("Unhandled element: {}", element.name);
                return std::nullopt;
            }
        },
        [&](dom::Text const &text) -> std::optional<ftxui::Element> {
            return hflow(ftxui::paragraph(ftxui::to_wstring(text.text)));
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
