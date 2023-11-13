// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "layout/layout.h"

#include "util/string.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <list>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

using namespace std::literals;

namespace layout {
namespace {

bool last_node_was_anonymous(LayoutBox const &box) {
    return !box.children.empty() && box.children.back().type == LayoutType::AnonymousBlock;
}

// https://www.w3.org/TR/CSS2/visuren.html#box-gen
std::optional<LayoutBox> create_tree(style::StyledNode const &node) {
    if (auto const *text = std::get_if<dom::Text>(&node.node)) {
        return LayoutBox{.node = &node, .type = LayoutType::Inline, .layout_text = std::string_view{text->text}};
    }

    assert(std::holds_alternative<dom::Element>(node.node));
    auto display = node.get_property<css::PropertyId::Display>();
    if (display == style::DisplayValue::None) {
        return std::nullopt;
    }

    LayoutBox box{&node, display == style::DisplayValue::Inline ? LayoutType::Inline : LayoutType::Block};

    for (auto const &child : node.children) {
        auto child_box = create_tree(child);
        if (!child_box) {
            continue;
        }

        if (child_box->type == LayoutType::Inline && box.type != LayoutType::Inline) {
            if (!last_node_was_anonymous(box)) {
                box.children.push_back(LayoutBox{nullptr, LayoutType::AnonymousBlock});
            }

            box.children.back().children.push_back(std::move(*child_box));
        } else {
            box.children.push_back(std::move(*child_box));
        }
    }

    return box;
}

constexpr bool is_non_space_whitespace(char c) {
    return c != ' ' && util::is_whitespace(c);
}

void collapse_whitespace(LayoutBox &box) {
    LayoutBox *last_text_box = nullptr;
    std::list<LayoutBox *> to_collapse{&box};

    auto starts_text_run = [&](LayoutBox const &l) {
        return last_text_box == nullptr && !std::holds_alternative<std::monostate>(l.layout_text);
    };
    auto ends_text_run = [&](LayoutBox const &l) {
        return last_text_box != nullptr && l.type != LayoutType::Inline;
    };
    auto needs_allocating_whitespace_collapsing = [](std::string_view text) {
        return (std::ranges::adjacent_find(
                        text, [](char a, char b) { return util::is_whitespace(a) && util::is_whitespace(b); })
                       != std::ranges::end(text))
                || (std::ranges::find_if(text, is_non_space_whitespace) != std::ranges::end(text));
    };
    auto perform_allocating_collapsing = [](LayoutBox &l) {
        // Copy the string, removing consecutive whitespace, and transforming all whitespace to spaces.
        auto text = std::get<std::string_view>(l.layout_text);
        std::string collapsed;
        std::ranges::unique_copy(text, std::back_inserter(collapsed), [](char a, char b) {
            return util::is_whitespace(a) && util::is_whitespace(b);
        });
        std::ranges::for_each(collapsed, [](char &c) { c = util::is_whitespace(c) ? ' ' : c; });
        l.layout_text = std::move(collapsed);
    };

    for (auto it = to_collapse.begin(); it != to_collapse.end(); ++it) {
        auto *current = *it;
        if (starts_text_run(*current)) {
            last_text_box = current;
            last_text_box->layout_text = util::trim_start(std::get<std::string_view>(last_text_box->layout_text));
        } else if (!std::holds_alternative<std::monostate>(current->layout_text)) {
            // Remove all but 1 trailing space.
            auto &text = std::get<std::string_view>(last_text_box->layout_text);
            auto last_non_whitespace_idx = text.find_last_not_of(" \n\r\f\v\t");
            if (last_non_whitespace_idx != std::string_view::npos) {
                auto trailing_whitespace_count = text.size() - last_non_whitespace_idx - 1;
                if (trailing_whitespace_count > 1) {
                    text.remove_suffix(trailing_whitespace_count - 1);
                }
            }

            if (last_text_box != nullptr
                    && last_text_box->text()
                               .transform([](auto sv) { return sv.empty() || util::is_whitespace(sv.back()); })
                               .value_or(false)) {
                current->layout_text = util::trim_start(std::get<std::string_view>(current->layout_text));
            }
            if (last_text_box != nullptr
                    && needs_allocating_whitespace_collapsing(std::get<std::string_view>(last_text_box->layout_text))) {
                perform_allocating_collapsing(*last_text_box);
            }
            last_text_box = current;
        } else if (ends_text_run(*current)) {
            last_text_box->layout_text = util::trim_end(std::get<std::string_view>(last_text_box->layout_text));
            if (needs_allocating_whitespace_collapsing(std::get<std::string_view>(last_text_box->layout_text))) {
                perform_allocating_collapsing(*last_text_box);
            }
            last_text_box = nullptr;
        }

        for (std::size_t child_idx = 0; child_idx < current->children.size(); ++child_idx) {
            auto insertion_point = it;
            std::advance(insertion_point, 1 + child_idx);
            to_collapse.insert(insertion_point, &current->children[child_idx]);
        }
    }

    if (last_text_box != nullptr) {
        last_text_box->layout_text = util::trim_end(std::get<std::string_view>(last_text_box->layout_text));
        if (needs_allocating_whitespace_collapsing(std::get<std::string_view>(last_text_box->layout_text))) {
            perform_allocating_collapsing(*last_text_box);
        }
    }
}

void calculate_left_and_right_margin(LayoutBox &box,
        geom::Rect const &parent,
        std::string_view margin_left,
        std::string_view margin_right,
        int const font_size,
        int const root_font_size) {
    if (margin_left == "auto" && margin_right == "auto") {
        int margin_px = (parent.width - box.dimensions.border_box().width) / 2;
        box.dimensions.margin.left = box.dimensions.margin.right = margin_px;
    } else if (margin_left == "auto" && margin_right != "auto") {
        box.dimensions.margin.right = to_px(margin_right, font_size, root_font_size);
        box.dimensions.margin.left = parent.width - box.dimensions.margin_box().width;
    } else if (margin_left != "auto" && margin_right == "auto") {
        box.dimensions.margin.left = to_px(margin_left, font_size, root_font_size);
        box.dimensions.margin.right = parent.width - box.dimensions.margin_box().width;
    } else {
        // TODO(mkiael): Compute margin depending on direction property
    }
}

// https://www.w3.org/TR/CSS2/visudet.html#blockwidth
void calculate_width_and_margin(
        LayoutBox &box, geom::Rect const &parent, int const font_size, int const root_font_size) {
    assert(box.node != nullptr);

    auto margin_top = box.get_property<css::PropertyId::MarginTop>();
    box.dimensions.margin.top = to_px(margin_top, font_size, root_font_size);

    auto margin_bottom = box.get_property<css::PropertyId::MarginBottom>();
    box.dimensions.margin.bottom = to_px(margin_bottom, font_size, root_font_size);

    auto margin_left = box.get_property<css::PropertyId::MarginLeft>();
    auto margin_right = box.get_property<css::PropertyId::MarginRight>();
    if (auto width = box.get_property<css::PropertyId::Width>(); !width.is_auto()) {
        box.dimensions.content.width = width.resolve(font_size, root_font_size, parent.width);
        calculate_left_and_right_margin(box, parent, margin_left, margin_right, font_size, root_font_size);
    } else {
        if (margin_left != "auto") {
            box.dimensions.margin.left = to_px(margin_left, font_size, root_font_size);
        }
        if (margin_right != "auto") {
            box.dimensions.margin.right = to_px(margin_right, font_size, root_font_size);
        }
        box.dimensions.content.width = parent.width - box.dimensions.margin_box().width;
    }

    if (auto min = box.get_property<css::PropertyId::MinWidth>()) {
        if (box.dimensions.content.width < *min) {
            box.dimensions.content.width = *min;
            calculate_left_and_right_margin(box, parent, margin_left, margin_right, font_size, root_font_size);
        }
    }

    if (auto max = box.get_property<css::PropertyId::MaxWidth>()) {
        if (box.dimensions.content.width > *max) {
            box.dimensions.content.width = *max;
            calculate_left_and_right_margin(box, parent, margin_left, margin_right, font_size, root_font_size);
        }
    }
}

void calculate_position(LayoutBox &box, geom::Rect const &parent) {
    auto const &d = box.dimensions;
    box.dimensions.content.x = parent.x + d.padding.left + d.border.left + d.margin.left;
    // Position below previous content in parent.
    box.dimensions.content.y = parent.y + parent.height + d.border.top + d.padding.top + d.margin.top;
}

void calculate_height(LayoutBox &box, int const font_size, int const root_font_size) {
    assert(box.node != nullptr);
    if (auto text = box.text()) {
        int lines = static_cast<int>(std::ranges::count(*text, '\n')) + 1;
        box.dimensions.content.height = lines * font_size;
    }

    if (auto height = box.get_property<css::PropertyId::Height>(); height != "auto") {
        box.dimensions.content.height = to_px(height, font_size, root_font_size);
    }

    if (auto min = box.get_property<css::PropertyId::MinHeight>(); min != "auto") {
        box.dimensions.content.height = std::max(box.dimensions.content.height, to_px(min, font_size, root_font_size));
    }

    if (auto max = box.get_property<css::PropertyId::MaxHeight>(); max != "none") {
        box.dimensions.content.height = std::min(box.dimensions.content.height, to_px(max, font_size, root_font_size));
    }
}

void calculate_padding(LayoutBox &box, int const font_size, int const root_font_size) {
    auto padding_left = box.get_property<css::PropertyId::PaddingLeft>();
    box.dimensions.padding.left = to_px(padding_left, font_size, root_font_size);

    auto padding_right = box.get_property<css::PropertyId::PaddingRight>();
    box.dimensions.padding.right = to_px(padding_right, font_size, root_font_size);

    auto padding_top = box.get_property<css::PropertyId::PaddingTop>();
    box.dimensions.padding.top = to_px(padding_top, font_size, root_font_size);

    auto padding_bottom = box.get_property<css::PropertyId::PaddingBottom>();
    box.dimensions.padding.bottom = to_px(padding_bottom, font_size, root_font_size);
}

// https://drafts.csswg.org/css-backgrounds/#the-border-width
// NOLINTNEXTLINE(cert-err58-cpp)
std::map<std::string_view, int> const border_width_keywords{
        {"thin", 3},
        {"medium", 5},
        {"thick", 7},
};

void calculate_border(LayoutBox &box, int const font_size, int const root_font_size) {
    auto as_px = [&](std::string_view border_width_property) {
        if (border_width_keywords.contains(border_width_property)) {
            return border_width_keywords.at(border_width_property);
        }

        return to_px(border_width_property, font_size, root_font_size);
    };

    if (box.get_property<css::PropertyId::BorderLeftStyle>() != style::BorderStyle::None) {
        auto border_width = box.get_property<css::PropertyId::BorderLeftWidth>();
        box.dimensions.border.left = as_px(border_width);
    }

    if (box.get_property<css::PropertyId::BorderRightStyle>() != style::BorderStyle::None) {
        auto border_width = box.get_property<css::PropertyId::BorderRightWidth>();
        box.dimensions.border.right = as_px(border_width);
    }

    if (box.get_property<css::PropertyId::BorderTopStyle>() != style::BorderStyle::None) {
        auto border_width = box.get_property<css::PropertyId::BorderTopWidth>();
        box.dimensions.border.top = as_px(border_width);
    }

    if (box.get_property<css::PropertyId::BorderBottomStyle>() != style::BorderStyle::None) {
        auto border_width = box.get_property<css::PropertyId::BorderBottomWidth>();
        box.dimensions.border.bottom = as_px(border_width);
    }
}

int text_width(std::string_view text, int const font_size) {
    return static_cast<int>(text.size()) * font_size / 2;
}

void layout(LayoutBox &box, geom::Rect const &bounds, int const root_font_size) {
    switch (box.type) {
        case LayoutType::Inline: {
            assert(box.node);
            auto font_size = box.get_property<css::PropertyId::FontSize>();
            calculate_padding(box, font_size, root_font_size);
            calculate_border(box, font_size, root_font_size);

            if (auto text = box.text()) {
                // TODO(robinlinden): Measure the text for real.
                box.dimensions.content.width = text_width(*text, font_size);
            }

            if (box.node->parent) {
                auto const &d = box.dimensions;
                box.dimensions.content.x = bounds.x + d.padding.left + d.border.left + d.margin.left;
                box.dimensions.content.y = bounds.y + d.border.top + d.padding.top + d.margin.top;
            }

            int last_child_end{};
            for (auto &child : box.children) {
                layout(child, box.dimensions.content.translated(last_child_end, 0), root_font_size);
                last_child_end += child.dimensions.margin_box().width;
                box.dimensions.content.height =
                        std::max(box.dimensions.content.height, child.dimensions.margin_box().height);
                box.dimensions.content.width += child.dimensions.margin_box().width;
            }
            calculate_height(box, font_size, root_font_size);
            return;
        }
        case LayoutType::Block: {
            assert(box.node);
            auto font_size = box.get_property<css::PropertyId::FontSize>();
            calculate_padding(box, font_size, root_font_size);
            calculate_border(box, font_size, root_font_size);
            calculate_width_and_margin(box, bounds, font_size, root_font_size);
            calculate_position(box, bounds);
            for (auto &child : box.children) {
                layout(child, box.dimensions.content, root_font_size);
                box.dimensions.content.height += child.dimensions.margin_box().height;
            }
            calculate_height(box, font_size, root_font_size);
            return;
        }
        case LayoutType::AnonymousBlock: {
            calculate_position(box, bounds);
            int last_child_end{};
            int current_line{};
            auto font_size = !box.children.empty() ? box.children[0].get_property<css::PropertyId::FontSize>() : 0;
            for (std::size_t i = 0; i < box.children.size(); ++i) {
                auto *child = &box.children[i];
                layout(*child,
                        box.dimensions.content.translated(last_child_end, current_line * font_size),
                        root_font_size);
                // TODO(robinlinden): Handle cases where the text isn't a direct child of the anonymous block.
                if (last_child_end + child->dimensions.margin_box().width > bounds.width) {
                    auto text = child->text();
                    if (text) {
                        std::size_t best_split_point = std::string_view::npos;
                        for (auto split_point = text->find(' '); split_point != std::string_view::npos;
                                split_point = text->find(' ', split_point + 1)) {
                            if (last_child_end + text_width(text->substr(0, split_point), font_size) > bounds.width) {
                                break;
                            }

                            best_split_point = split_point;
                        }

                        if (best_split_point != std::string_view::npos) {
                            child->dimensions.content.width = text_width(text->substr(0, best_split_point), font_size);
                            auto bonus_child = *child;
                            bonus_child.layout_text = std::string{text->substr(best_split_point + 1)};
                            box.children.insert(box.children.begin() + i + 1, std::move(bonus_child));
                            current_line += 1;
                            last_child_end = 0;

                            // Adding a child may have had to relocate the container content.
                            child = &box.children[i];
                            child->layout_text = std::string{text->substr(0, best_split_point)};
                        } else {
                            child->dimensions.content.width = text_width(*text, font_size);
                            last_child_end += child->dimensions.margin_box().width;
                        }
                    }
                } else {
                    last_child_end += child->dimensions.margin_box().width;
                }
                box.dimensions.content.height = std::max(
                        box.dimensions.content.height, child->dimensions.margin_box().height * (current_line + 1));
                box.dimensions.content.width = std::max(
                        box.dimensions.content.width, std::max(last_child_end, child->dimensions.content.width));
            }
            return;
        }
    }
}

} // namespace

std::optional<LayoutBox> create_layout(style::StyledNode const &node, int width) {
    auto tree = create_tree(node);
    if (!tree) {
        return {};
    }

    collapse_whitespace(*tree);

    layout(*tree, {0, 0, width, 0}, node.get_property<css::PropertyId::FontSize>());
    return *tree;
}

} // namespace layout
