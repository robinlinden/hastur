// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "tui/tui.h"

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/screen/string.hpp>

#include <cstdlib>
#include <string>
#include <variant>

namespace tui {
namespace {

ftxui::Element element_from_node(layout::LayoutBox const &box);

ftxui::Elements parse_children(layout::LayoutBox const &box) {
    ftxui::Elements children;
    for (auto const &child : box.children) {
        children.push_back(element_from_node(child));
    };

    return children;
}

ftxui::Element element_from_node(layout::LayoutBox const &box) {
    switch (box.type) {
        case layout::LayoutType::Inline: {
            if (auto text = std::get_if<dom::Text>(&box.node->node.get().data)) {
                return hflow(ftxui::paragraph(text->text));
            }
            return hbox(parse_children(box));
        }
        case layout::LayoutType::AnonymousBlock:
        case layout::LayoutType::Block: {
            return flex(vbox(parse_children(box)));
        }
    }
    std::abort(); // unreachable
}

} // namespace

std::string render(layout::LayoutBox const &root) {
    auto document = element_from_node(root);
    document = document | ftxui::size(ftxui::WIDTH, ftxui::LESS_THAN, 80);
    auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80), ftxui::Dimension::Fixed(10));
    ftxui::Render(screen, document);
    return screen.ToString();
}

} // namespace tui
