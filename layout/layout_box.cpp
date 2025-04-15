// SPDX-FileCopyrightText: 2021-2025 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "layout/layout_box.h"

#include "css/property_id.h"
#include "dom/dom.h"
#include "geom/geom.h"
#include "style/styled_node.h"

#include <cassert>
#include <cstdint>
#include <format>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

using namespace std::literals;

namespace layout {
namespace {

std::string_view layout_type(LayoutBox const &box) {
    if (box.is_anonymous_block()) {
        return "ablock";
    }

    auto const display = box.get_property<css::PropertyId::Display>();
    assert(display.has_value());
    assert(display->outer == style::Display::Outer::Block || display->outer == style::Display::Outer::Inline);
    if (display->outer == style::Display::Outer::Inline) {
        return "inline";
    }

    return "block";
}

std::string to_str(geom::Rect const &rect) {
    return std::format("{{{},{},{},{}}}", rect.x, rect.y, rect.width, rect.height);
}

std::string to_str(geom::EdgeSize const &edge) {
    return std::format("{{{},{},{},{}}}", edge.top, edge.right, edge.bottom, edge.left);
}

// NOLINTNEXTLINE(misc-no-recursion)
void print_box(LayoutBox const &box, std::ostream &os, std::uint8_t depth = 0) {
    for (std::uint8_t i = 0; i < depth; ++i) {
        os << "  ";
    }

    if (box.node != nullptr) {
        if (auto const *element = std::get_if<dom::Element>(&box.node->node)) {
            os << element->name << '\n';
        } else {
            auto text = box.text();
            assert(text.has_value());
            os << text.value() << '\n';
        }

        for (std::uint8_t i = 0; i < depth; ++i) {
            os << "  ";
        }
    }

    auto const &d = box.dimensions;
    os << layout_type(box) << " " << to_str(d.content) << " " << to_str(d.padding) << " " << to_str(d.margin) << '\n';
    for (auto const &child : box.children) {
        print_box(child, os, depth + 1);
    }
}

} // namespace

std::optional<std::string_view> LayoutBox::text() const {
    struct Visitor {
        std::optional<std::string_view> operator()(std::monostate) { return std::nullopt; }
        std::optional<std::string_view> operator()(std::string const &s) { return s; }
        std::optional<std::string_view> operator()(std::string_view const &s) { return s; }
    };
    return std::visit(Visitor{}, layout_text);
}

// NOLINTNEXTLINE(misc-no-recursion)
LayoutBox const *box_at_position(LayoutBox const &box, geom::Position p) {
    if (!box.dimensions.contains(p)) {
        return nullptr;
    }

    for (auto const &child : box.children) {
        if (auto const *maybe = box_at_position(child, p)) {
            return maybe;
        }
    }

    if (box.is_anonymous_block()) {
        return nullptr;
    }

    return &box;
}

std::string to_string(LayoutBox const &box) {
    std::stringstream ss;
    print_box(box, ss);
    return std::move(ss).str();
}

} // namespace layout
