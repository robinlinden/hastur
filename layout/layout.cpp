// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "layout/layout.h"

#include <algorithm>
#include <cassert>
#include <charconv>
#include <optional>
#include <sstream>
#include <string_view>
#include <utility>
#include <variant>

using namespace std::literals;

namespace layout {
namespace {

constexpr int kDefaultFontSizePx = 10;
constexpr std::string_view kDefaultFontSize{"10px"};

template<class... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};

// Not needed as of C++20, but gcc 10 won't work without it.
template<class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

bool last_node_was_anonymous(LayoutBox const &box) {
    return !box.children.empty() && box.children.back().type == LayoutType::AnonymousBlock;
}

std::optional<LayoutBox> create_tree(style::StyledNode const &node) {
    return std::visit(Overloaded{
                              [&node](dom::Element const &) -> std::optional<LayoutBox> {
                                  auto display = style::get_property(node, "display");
                                  if (display && *display == "none") {
                                      return std::nullopt;
                                  }

                                  LayoutBox box{&node, display == "inline" ? LayoutType::Inline : LayoutType::Block};

                                  for (auto const &child : node.children) {
                                      auto child_box = create_tree(child);
                                      if (!child_box)
                                          continue;

                                      if (child_box->type == LayoutType::Inline) {
                                          if (!last_node_was_anonymous(box)) {
                                              box.children.push_back(LayoutBox{nullptr, LayoutType::AnonymousBlock});
                                          }

                                          box.children.back().children.push_back(std::move(*child_box));
                                      } else {
                                          box.children.push_back(std::move(*child_box));
                                      }
                                  }

                                  return box;
                              },
                              [&node](dom::Text const &) -> std::optional<LayoutBox> {
                                  return LayoutBox{&node, LayoutType::Inline};
                              },
                      },
            node.node);
}

// TODO(robinlinden):
// * margin, border, etc.
// * Not all measurements have to be in pixels.
// * %, rem
int to_px(std::string_view property, int const font_size) {
    int res{};
    std::from_chars(property.data(), property.data() + property.size(), res);
    if (property.ends_with("em")) {
        res *= font_size;
    }

    return res;
}

void calculate_left_and_right_margin(LayoutBox &box,
        geom::Rect const &parent,
        std::string_view margin_left,
        std::string_view margin_right,
        int const font_size) {
    if (margin_left == "auto" && margin_right == "auto") {
        int margin_px = (parent.width - box.dimensions.border_box().width) / 2;
        box.dimensions.margin.left = box.dimensions.margin.right = margin_px;
    } else if (margin_left == "auto" && margin_right != "auto") {
        box.dimensions.margin.right = to_px(margin_right, font_size);
        box.dimensions.margin.left = parent.width - box.dimensions.margin_box().width;
    } else if (margin_left != "auto" && margin_right == "auto") {
        box.dimensions.margin.left = to_px(margin_left, font_size);
        box.dimensions.margin.right = parent.width - box.dimensions.margin_box().width;
    } else {
        // TODO(mkiael): Compute margin depending on direction property
    }
}

// https://www.w3.org/TR/CSS2/visudet.html#blockwidth
void calculate_width_and_margin(LayoutBox &box, geom::Rect const &parent, int const font_size) {
    assert(box.node != nullptr);

    if (std::holds_alternative<dom::Text>(box.node->node)) {
        // TODO(robinlinden): Measure the text for real.
        auto text_node = std::get<dom::Text>(box.node->node);
        box.dimensions.content.width = std::min(parent.width, static_cast<int>(text_node.text.size()) * font_size / 2);
        return;
    }

    if (auto margin_top = style::get_property(*box.node, "margin-top")) {
        box.dimensions.margin.top = to_px(*margin_top, font_size);
    }

    if (auto margin_bottom = style::get_property(*box.node, "margin-bottom")) {
        box.dimensions.margin.bottom = to_px(*margin_bottom, font_size);
    }

    auto width = style::get_property_or(*box.node, "width", "auto");
    auto margin_left = style::get_property_or(*box.node, "margin-left", "0");
    auto margin_right = style::get_property_or(*box.node, "margin-right", "0");
    if (width == "auto") {
        if (margin_left != "auto") {
            box.dimensions.margin.left = to_px(margin_left, font_size);
        }
        if (margin_right != "auto") {
            box.dimensions.margin.right = to_px(margin_right, font_size);
        }
        box.dimensions.content.width = parent.width - box.dimensions.margin_box().width;
    } else {
        box.dimensions.content.width = to_px(width, font_size);
        calculate_left_and_right_margin(box, parent, margin_left, margin_right, font_size);
    }

    if (auto min = style::get_property(*box.node, "min-width")) {
        int min_width_px = to_px(*min, font_size);
        if (box.dimensions.content.width < min_width_px) {
            box.dimensions.content.width = min_width_px;
            calculate_left_and_right_margin(box, parent, margin_left, margin_right, font_size);
        }
    }

    if (auto max = style::get_property(*box.node, "max-width")) {
        int max_width_px = to_px(*max, font_size);
        if (box.dimensions.content.width > max_width_px) {
            box.dimensions.content.width = max_width_px;
            calculate_left_and_right_margin(box, parent, margin_left, margin_right, font_size);
        }
    }
}

void calculate_position(LayoutBox &box, geom::Rect const &parent) {
    auto const &d = box.dimensions;
    box.dimensions.content.x = parent.x + d.padding.left + d.border.left + d.margin.left;
    // Position below previous content in parent.
    box.dimensions.content.y = parent.y + parent.height + d.border.top + d.padding.top + d.margin.top;
}

void calculate_height(LayoutBox &box, int const font_size) {
    assert(box.node != nullptr);
    if (std::holds_alternative<dom::Text>(box.node->node)) {
        box.dimensions.content.height = font_size;
    }

    if (auto height = style::get_property(*box.node, "height")) {
        box.dimensions.content.height = to_px(*height, font_size);
    }

    if (auto min = style::get_property(*box.node, "min-height")) {
        box.dimensions.content.height = std::max(box.dimensions.content.height, to_px(*min, font_size));
    }

    if (auto max = style::get_property(*box.node, "max-height")) {
        box.dimensions.content.height = std::min(box.dimensions.content.height, to_px(*max, font_size));
    }
}

void calculate_padding(LayoutBox &box, int const font_size) {
    if (auto padding_left = style::get_property(*box.node, "padding-left")) {
        box.dimensions.padding.left = to_px(*padding_left, font_size);
    }

    if (auto padding_right = style::get_property(*box.node, "padding-right")) {
        box.dimensions.padding.right = to_px(*padding_right, font_size);
    }

    if (auto padding_top = style::get_property(*box.node, "padding-top")) {
        box.dimensions.padding.top = to_px(*padding_top, font_size);
    }

    if (auto padding_bottom = style::get_property(*box.node, "padding-bottom")) {
        box.dimensions.padding.bottom = to_px(*padding_bottom, font_size);
    }
}

void calculate_border(LayoutBox &box, int const font_size) {
    std::string_view default_style = "none";

    // TODO(mkiael): Change to "meduim" when this is supported
    std::string_view default_width = "3px";

    if (style::get_property_or(*box.node, "border-left-style", default_style) != default_style) {
        auto border_width = style::get_property_or(*box.node, "border-left-width", default_width);
        box.dimensions.border.left = to_px(border_width, font_size);
    }

    if (style::get_property_or(*box.node, "border-right-style", default_style) != default_style) {
        auto border_width = style::get_property_or(*box.node, "border-right-width", default_width);
        box.dimensions.border.right = to_px(border_width, font_size);
    }

    if (style::get_property_or(*box.node, "border-top-style", default_style) != default_style) {
        auto border_width = style::get_property_or(*box.node, "border-top-width", default_width);
        box.dimensions.border.top = to_px(border_width, font_size);
    }

    if (style::get_property_or(*box.node, "border-bottom-style", default_style) != default_style) {
        auto border_width = style::get_property_or(*box.node, "border-bottom-width", default_width);
        box.dimensions.border.bottom = to_px(border_width, font_size);
    }
}

void layout(LayoutBox &box, geom::Rect const &bounds) {
    switch (box.type) {
        case LayoutType::Inline:
        case LayoutType::Block: {
            // TODO(robinlinden): font-size should be inherited.
            auto font_size =
                    to_px(style::get_property_or(*box.node, "font-size", kDefaultFontSize), kDefaultFontSizePx);
            calculate_padding(box, font_size);
            calculate_border(box, font_size);
            calculate_width_and_margin(box, bounds, font_size);
            calculate_position(box, bounds);
            for (auto &child : box.children) {
                layout(child, box.dimensions.content);
                box.dimensions.content.height += child.dimensions.margin_box().height;
            }
            calculate_height(box, font_size);
            return;
        }
        // TODO(robinlinden): This needs to place its children side-by-side.
        // TODO(robinlinden): Children wider than the available area need to be split across multiple lines.
        case LayoutType::AnonymousBlock: {
            box.dimensions.content.width = bounds.width;
            calculate_position(box, bounds);
            for (auto &child : box.children) {
                layout(child, box.dimensions.content);
                box.dimensions.content.height += child.dimensions.margin_box().height;
            }
            return;
        }
    }
}

std::string_view to_str(LayoutType type) {
    switch (type) {
        case LayoutType::Inline:
            return "inline";
        case LayoutType::Block:
            return "block";
        case LayoutType::AnonymousBlock:
            return "ablock";
    }
    assert(false);
    std::abort();
}

std::string_view to_str(dom::Node const &node) {
    return std::visit(Overloaded{
                              [](dom::Element const &element) -> std::string_view { return element.name; },
                              [](dom::Text const &text) -> std::string_view { return text.text; },
                      },
            node);
}

std::string to_str(geom::Rect const &rect) {
    std::stringstream ss;
    ss << "{" << rect.x << "," << rect.y << "," << rect.width << "," << rect.height << "}";
    return ss.str();
}

std::string to_str(geom::EdgeSize const &edge) {
    std::stringstream ss;
    ss << "{" << edge.top << "," << edge.right << "," << edge.bottom << "," << edge.left << "}";
    return ss.str();
}

void print_box(LayoutBox const &box, std::ostream &os, uint8_t depth = 0) {
    for (int8_t i = 0; i < depth; ++i) {
        os << "  ";
    }

    if (box.node != nullptr) {
        os << to_str(box.node->node) << '\n';
        for (int8_t i = 0; i < depth; ++i) {
            os << "  ";
        }
    }

    auto const &d = box.dimensions;
    os << to_str(box.type) << " " << to_str(d.content) << " " << to_str(d.padding) << " " << to_str(d.margin) << '\n';
    for (auto const &child : box.children) {
        print_box(child, os, depth + 1);
    }
}

} // namespace

geom::Rect BoxModel::padding_box() const {
    return content.expanded(padding);
}

geom::Rect BoxModel::border_box() const {
    return padding_box().expanded(border);
}

geom::Rect BoxModel::margin_box() const {
    return border_box().expanded(margin);
}

LayoutBox create_layout(style::StyledNode const &node, int width) {
    auto tree = create_tree(node);
    layout(*tree, {0, 0, width, 0});
    return *tree;
}

LayoutBox const *box_at_position(LayoutBox const &box, geom::Position p) {
    if (!box.dimensions.contains(p)) {
        return nullptr;
    }

    for (auto const &child : box.children) {
        if (auto maybe = box_at_position(child, p)) {
            return maybe;
        }
    }

    if (box.type == LayoutType::AnonymousBlock) {
        return nullptr;
    }

    return &box;
}

std::string to_string(LayoutBox const &box) {
    std::stringstream ss;
    print_box(box, ss);
    return ss.str();
}

} // namespace layout
