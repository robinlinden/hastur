// SPDX-FileCopyrightText: 2021-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "tui/tui.h"

#include "css/property_id.h"
#include "layout/layout_box.h"
#include "style/styled_node.h"

#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/screen.hpp>

#include <cassert>
#include <string>

namespace tui {
namespace {

ftxui::Element element_from_node(layout::LayoutBox const &box);

// NOLINTNEXTLINE(misc-no-recursion)
ftxui::Elements parse_children(layout::LayoutBox const &box) {
    ftxui::Elements children;
    for (auto const &child : box.children) {
        children.push_back(element_from_node(child));
    };

    return children;
}

// NOLINTNEXTLINE(misc-no-recursion)
ftxui::Element element_from_node(layout::LayoutBox const &box) {
    if (box.is_anonymous_block()) {
        return flex(vbox(parse_children(box)));
    }

    auto const display = box.get_property<css::PropertyId::Display>();
    assert(display.has_value());
    assert(display == style::Display::inline_flow() || display == style::Display::block_flow());
    if (display == style::Display::inline_flow()) {
        if (auto text = box.text()) {
            return ftxui::paragraph(std::string{*text});
        }
        return hbox(parse_children(box));
    }

    return flex(vbox(parse_children(box)));
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
