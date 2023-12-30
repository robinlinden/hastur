// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "tui/tui.h"

#include "layout/layout_box.h"

#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/screen.hpp>

#include <cstdlib>
#include <string>

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
            if (auto text = box.text()) {
                return ftxui::paragraph(std::string{*text});
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
