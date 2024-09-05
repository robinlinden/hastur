// SPDX-FileCopyrightText: 2021-2024 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "layout/layout.h"

#include "layout/layout_box.h"

#include "css/property_id.h"
#include "dom/dom.h"
#include "geom/geom.h"
#include "style/styled_node.h"
#include "style/unresolved_value.h"
#include "type/naive.h"
#include "type/type.h"
#include "util/string.h"

#include <fmt/ranges.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iterator>
#include <list>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

using namespace std::literals;

namespace layout {
namespace {

class Layouter {
public:
    Layouter(int root_font_size, type::IType const &type) : root_font_size_{root_font_size}, type_{type} {}

    void layout(LayoutBox &, geom::Rect const &bounds) const;

private:
    int root_font_size_;
    type::IType const &type_;

    void layout_inline(LayoutBox &, geom::Rect const &bounds) const;
    void layout_block(LayoutBox &, geom::Rect const &bounds) const;
    void layout_anonymous_block(LayoutBox &, geom::Rect const &bounds) const;

    void calculate_left_and_right_margin(LayoutBox &,
            geom::Rect const &parent,
            style::UnresolvedValue margin_left,
            style::UnresolvedValue margin_right,
            int font_size) const;
    void calculate_width_and_margin(LayoutBox &, geom::Rect const &parent, int font_size) const;
    void calculate_height(LayoutBox &, int font_size) const;
    void calculate_padding(LayoutBox &, int font_size) const;
    void calculate_border(LayoutBox &, int font_size) const;
    std::optional<std::shared_ptr<type::IFont const>> find_font(std::span<std::string_view const> font_families) const;
};

bool last_node_was_anonymous(LayoutBox const &box) {
    return !box.children.empty() && box.children.back().is_anonymous_block();
}

// https://www.w3.org/TR/CSS2/visuren.html#box-gen
// NOLINTNEXTLINE(misc-no-recursion)
std::optional<LayoutBox> create_tree(style::StyledNode const &node) {
    if (auto const *text = std::get_if<dom::Text>(&node.node)) {
        return LayoutBox{.node = &node, .layout_text = std::string_view{text->text}};
    }

    assert(std::holds_alternative<dom::Element>(node.node));
    auto display = node.get_property<css::PropertyId::Display>();
    if (!display.has_value()) {
        return std::nullopt;
    }

    if (auto const &element = std::get<dom::Element>(node.node); element.name == "img"sv) {
        if (auto alt = element.attributes.find("alt"sv); alt != element.attributes.end()) {
            return LayoutBox{.node = &node, .layout_text = std::string_view{alt->second}};
        }
    }

    LayoutBox box{&node};

    for (auto const &child : node.children) {
        auto child_box = create_tree(child);
        if (!child_box) {
            continue;
        }

        auto child_display = child.get_property<css::PropertyId::Display>();
        assert(child_display.has_value());
        if (child_display == style::Display::inline_flow() && display != style::Display::inline_flow()) {
            if (!last_node_was_anonymous(box)) {
                box.children.push_back(LayoutBox{nullptr});
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

// NOLINTNEXTLINE(misc-no-recursion)
void remove_empty_text_boxes(LayoutBox &box) {
    for (auto it = box.children.begin(); it != box.children.end();) {
        auto text = it->text();
        if (text.has_value() && text->empty()) {
            it = box.children.erase(it);
            continue;
        }

        remove_empty_text_boxes(*it);
        if (it->is_anonymous_block() && it->children.empty()) {
            it = box.children.erase(it);
            continue;
        }

        ++it;
    }
}

void collapse_whitespace(LayoutBox &box) {
    LayoutBox *last_text_box = nullptr;
    std::list<LayoutBox *> to_collapse{&box};

    auto starts_text_run = [](LayoutBox const &l) {
        return !std::holds_alternative<std::monostate>(l.layout_text);
    };
    auto ends_text_run = [](LayoutBox const &l) {
        return l.is_anonymous_block() || l.get_property<css::PropertyId::Display>() != style::Display::inline_flow();
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
        auto &current = **it;
        if (last_text_box == nullptr && starts_text_run(current)) {
            last_text_box = &current;
            auto &text = std::get<std::string_view>(last_text_box->layout_text);
            text = util::trim_start(text);

            // A completely empty text box can't start a text run.
            if (text.empty()) {
                last_text_box = nullptr;
            }
        } else if (last_text_box != nullptr && !std::holds_alternative<std::monostate>(current.layout_text)) {
            // Remove all but 1 trailing space.
            auto &text = std::get<std::string_view>(last_text_box->layout_text);
            auto last_non_whitespace_idx = text.find_last_not_of(" \n\r\f\v\t");
            if (last_non_whitespace_idx != std::string_view::npos) {
                auto trailing_whitespace_count = text.size() - last_non_whitespace_idx - 1;
                if (trailing_whitespace_count > 1) {
                    text.remove_suffix(trailing_whitespace_count - 1);
                }
            }

            if (last_text_box->text()
                            .transform([](auto sv) { return sv.empty() || util::is_whitespace(sv.back()); })
                            .value_or(false)) {
                current.layout_text = util::trim_start(std::get<std::string_view>(current.layout_text));
            }

            if (needs_allocating_whitespace_collapsing(std::get<std::string_view>(last_text_box->layout_text))) {
                perform_allocating_collapsing(*last_text_box);
            }

            last_text_box = &current;
        } else if (last_text_box != nullptr && ends_text_run(current)) {
            last_text_box->layout_text = util::trim_end(std::get<std::string_view>(last_text_box->layout_text));
            if (needs_allocating_whitespace_collapsing(std::get<std::string_view>(last_text_box->layout_text))) {
                perform_allocating_collapsing(*last_text_box);
            }
            last_text_box = nullptr;
        }

        for (std::size_t child_idx = 0; child_idx < current.children.size(); ++child_idx) {
            auto insertion_point = it;
            std::advance(insertion_point, 1 + child_idx);
            to_collapse.insert(insertion_point, &current.children[child_idx]);
        }
    }

    if (last_text_box != nullptr) {
        last_text_box->layout_text = util::trim_end(std::get<std::string_view>(last_text_box->layout_text));
        if (needs_allocating_whitespace_collapsing(std::get<std::string_view>(last_text_box->layout_text))) {
            perform_allocating_collapsing(*last_text_box);
        }
    }

    remove_empty_text_boxes(box);
}

// NOLINTNEXTLINE(misc-no-recursion)
void apply_text_transforms(LayoutBox &box) {
    if (std::holds_alternative<std::string>(box.layout_text)
            || std::holds_alternative<std::string_view>(box.layout_text)) {
        if (auto transform = box.get_property<css::PropertyId::TextTransform>();
                transform && *transform != style::TextTransform::None) {
            if (std::holds_alternative<std::string_view>(box.layout_text)) {
                box.layout_text = std::string{std::get<std::string_view>(box.layout_text)};
            }

            auto &text = std::get<std::string>(box.layout_text);

            // TODO(robinlinden): FullWidth, FullSizeKana.
            // TODO(robinlinden): Handle language-specific cases.
            switch (*transform) {
                case style::TextTransform::FullWidth:
                case style::TextTransform::FullSizeKana:
                case style::TextTransform::None:
                    break;
                case style::TextTransform::Uppercase:
                    std::ranges::for_each(text, [](char &c) { c = util::uppercased(c); });
                    break;
                case style::TextTransform::Lowercase:
                    std::ranges::for_each(text, [](char &c) { c = util::lowercased(c); });
                    break;
                case style::TextTransform::Capitalize:
                    std::ranges::for_each(text, [first = true](char &c) mutable {
                        if (first && util::is_alpha(c)) {
                            first = false;
                            c = util::uppercased(c);
                        } else if (!first && !util::is_alpha(c)) {
                            first = true;
                        } else {
                            c = util::lowercased(c);
                        }
                    });
                    break;
            }
        }
    }

    for (auto &child : box.children) {
        apply_text_transforms(child);
    }
}

void calculate_position(LayoutBox &box, geom::Rect const &parent) {
    auto const &d = box.dimensions;
    box.dimensions.content.x = parent.x + d.padding.left + d.border.left + d.margin.left;
    // Position below previous content in parent.
    box.dimensions.content.y = parent.y + parent.height + d.border.top + d.padding.top + d.margin.top;
}

// NOLINTNEXTLINE(misc-no-recursion)
void Layouter::layout(LayoutBox &box, geom::Rect const &bounds) const {
    if (box.is_anonymous_block()) {
        layout_anonymous_block(box, bounds);
        return;
    }

    // Nodes w/ `display: none` aren't added to the tree and shouldn't end up here.
    auto display = box.get_property<css::PropertyId::Display>();
    assert(display.has_value());
    if (display == style::Display::inline_flow()) {
        layout_inline(box, bounds);
        return;
    }

    layout_block(box, bounds);
}

type::Weight to_type(std::optional<style::FontWeight> const &weight) {
    if (!weight || weight->value < style::FontWeight::kBold) {
        return type::Weight::Normal;
    }

    return type::Weight::Bold;
}

// NOLINTNEXTLINE(misc-no-recursion)
void Layouter::layout_inline(LayoutBox &box, geom::Rect const &bounds) const {
    assert(box.node);
    auto font_size = box.get_property<css::PropertyId::FontSize>();
    calculate_padding(box, font_size);
    calculate_border(box, font_size);

    if (auto text = box.text()) {
        auto font_families = box.get_property<css::PropertyId::FontFamily>();
        auto weight = to_type(box.get_property<css::PropertyId::FontWeight>());
        auto font = find_font(font_families);
        if (font) {
            box.dimensions.content.width = (*font)->measure(*text, type::Px{font_size}, weight).width;
        } else {
            spdlog::warn("No font found for font-families: {}", fmt::join(font_families, ", "));
            box.dimensions.content.width = type::NaiveFont{}.measure(*text, type::Px{font_size}, weight).width;
        }
    }

    if (box.node->parent != nullptr) {
        auto const &d = box.dimensions;
        box.dimensions.content.x = bounds.x + d.padding.left + d.border.left + d.margin.left;
        box.dimensions.content.y = bounds.y + d.border.top + d.padding.top + d.margin.top;
    }

    int last_child_end{};
    for (auto &child : box.children) {
        layout(child, box.dimensions.content.translated(last_child_end, 0));
        last_child_end += child.dimensions.margin_box().width;
        box.dimensions.content.height = std::max(box.dimensions.content.height, child.dimensions.margin_box().height);
        box.dimensions.content.width += child.dimensions.margin_box().width;
    }
    calculate_height(box, font_size);
}

// NOLINTNEXTLINE(misc-no-recursion)
void Layouter::layout_block(LayoutBox &box, geom::Rect const &bounds) const {
    assert(box.node);
    auto font_size = box.get_property<css::PropertyId::FontSize>();
    calculate_padding(box, font_size);
    calculate_border(box, font_size);
    calculate_width_and_margin(box, bounds, font_size);
    calculate_position(box, bounds);
    for (auto &child : box.children) {
        layout(child, box.dimensions.content);
        box.dimensions.content.height += child.dimensions.margin_box().height;
    }
    calculate_height(box, font_size);
}

// NOLINTNEXTLINE(misc-no-recursion)
void Layouter::layout_anonymous_block(LayoutBox &box, geom::Rect const &bounds) const {
    calculate_position(box, bounds);
    int last_child_end{};
    int current_line{};
    auto font_size = type::Px{!box.children.empty() ? box.children[0].get_property<css::PropertyId::FontSize>() : 0};
    auto font_families = !box.children.empty() ? box.children[0].get_property<css::PropertyId::FontFamily>()
                                               : std::vector<std::string_view>{};

    auto maybe_font = find_font(font_families);
    if (!maybe_font) {
        spdlog::warn("No font found for font-families: {}", fmt::join(font_families, ", "));
        maybe_font = std::make_shared<type::NaiveFont>();
    }
    auto font = *maybe_font;

    auto weight =
            to_type(!box.children.empty() ? box.children[0].get_property<css::PropertyId::FontWeight>() : std::nullopt);

    for (std::size_t i = 0; i < box.children.size(); ++i) {
        auto *child = &box.children[i];
        layout(*child, box.dimensions.content.translated(last_child_end, current_line * font_size.v));

        // TODO(robinlinden): This needs to get along better with whitespace
        // collapsing. A <br> followed by a whitespace will be lead to a leading
        // space on the new line.
        if (auto const *ele = std::get_if<dom::Element>(&child->node->node); ele != nullptr && ele->name == "br"sv) {
            current_line += 1;
            last_child_end = 0;
            continue;
        }

        // TODO(robinlinden): Handle cases where the text isn't a direct child of the anonymous block.
        if (last_child_end + child->dimensions.margin_box().width > bounds.width) {
            auto maybe_text = child->text();
            if (maybe_text.has_value()) {
                std::string_view text = *maybe_text;

                std::size_t best_split_point = std::string_view::npos;
                for (auto split_point = text.find(' '); split_point != std::string_view::npos;
                        split_point = text.find(' ', split_point + 1)) {
                    if (last_child_end + font->measure(text.substr(0, split_point), font_size, weight).width
                            > bounds.width) {
                        break;
                    }

                    best_split_point = split_point;
                }

                if (best_split_point != std::string_view::npos) {
                    child->dimensions.content.width =
                            font->measure(text.substr(0, best_split_point), font_size, weight).width;
                    auto bonus_child = *child;
                    bonus_child.layout_text = std::string{text.substr(best_split_point + 1)};
                    box.children.insert(box.children.begin() + i + 1, std::move(bonus_child));
                    current_line += 1;
                    last_child_end = 0;

                    // Adding a child may have had to relocate the container content.
                    child = &box.children[i];
                    child->layout_text = std::string{text.substr(0, best_split_point)};
                } else {
                    child->dimensions.content.width = font->measure(text, font_size, weight).width;
                    last_child_end += child->dimensions.margin_box().width;
                }
            }
        } else {
            last_child_end += child->dimensions.margin_box().width;
        }
        box.dimensions.content.height =
                std::max(box.dimensions.content.height, child->dimensions.margin_box().height * (current_line + 1));
        box.dimensions.content.width =
                std::max(box.dimensions.content.width, std::max(last_child_end, child->dimensions.content.width));
    }
}

void Layouter::calculate_left_and_right_margin(LayoutBox &box,
        geom::Rect const &parent,
        style::UnresolvedValue margin_left,
        style::UnresolvedValue margin_right,
        int const font_size) const {
    if (margin_left.is_auto() && margin_right.is_auto()) {
        int margin_px = (parent.width - box.dimensions.border_box().width) / 2;
        box.dimensions.margin.left = box.dimensions.margin.right = margin_px;
    } else if (margin_left.is_auto() && !margin_right.is_auto()) {
        box.dimensions.margin.right = margin_right.resolve(font_size, root_font_size_);
        box.dimensions.margin.left = parent.width - box.dimensions.margin_box().width;
    } else if (!margin_left.is_auto() && margin_right.is_auto()) {
        box.dimensions.margin.left = margin_left.resolve(font_size, root_font_size_);
        box.dimensions.margin.right = parent.width - box.dimensions.margin_box().width;
    } else {
        // TODO(mkiael): Compute margin depending on direction property
    }
}

// https://www.w3.org/TR/CSS2/visudet.html#blockwidth
void Layouter::calculate_width_and_margin(LayoutBox &box, geom::Rect const &parent, int const font_size) const {
    assert(box.node != nullptr);

    auto &margins = box.dimensions.margin;
    margins.top = box.get_property<css::PropertyId::MarginTop>().resolve(font_size, root_font_size_);
    margins.bottom = box.get_property<css::PropertyId::MarginBottom>().resolve(font_size, root_font_size_);

    auto margin_left = box.get_property<css::PropertyId::MarginLeft>();
    auto margin_right = box.get_property<css::PropertyId::MarginRight>();
    auto width = box.get_property<css::PropertyId::Width>();
    std::optional<int> resolved_width;
    if (!width.is_auto()) {
        resolved_width = width.try_resolve(font_size, root_font_size_, parent.width);
    }

    if (resolved_width) {
        box.dimensions.content.width = *resolved_width;
        calculate_left_and_right_margin(box, parent, margin_left, margin_right, font_size);
    } else {
        if (!margin_left.is_auto()) {
            margins.left = margin_left.resolve(font_size, root_font_size_);
        }
        if (!margin_right.is_auto()) {
            margins.right = margin_right.resolve(font_size, root_font_size_);
        }
        box.dimensions.content.width = parent.width - box.dimensions.margin_box().width;
    }

    if (auto min = box.get_property<css::PropertyId::MinWidth>(); !min.is_auto()) {
        auto resolved = min.resolve(font_size, root_font_size_, parent.width);
        if (box.dimensions.content.width < resolved) {
            box.dimensions.content.width = resolved;
            calculate_left_and_right_margin(box, parent, margin_left, margin_right, font_size);
        }
    }

    auto max = box.get_property<css::PropertyId::MaxWidth>();
    std::optional<int> resolved_max;
    if (!max.is_none()) {
        resolved_max = max.try_resolve(font_size, root_font_size_, parent.width);
    }

    if (resolved_max) {
        if (box.dimensions.content.width > *resolved_max) {
            box.dimensions.content.width = *resolved_max;
            calculate_left_and_right_margin(box, parent, margin_left, margin_right, font_size);
        }
    }
}

void Layouter::calculate_height(LayoutBox &box, int const font_size) const {
    assert(box.node != nullptr);
    auto &content = box.dimensions.content;

    if (auto text = box.text()) {
        int lines = static_cast<int>(std::ranges::count(*text, '\n')) + 1;
        content.height = lines * font_size;
    }

    if (auto height = box.get_property<css::PropertyId::Height>(); !height.is_auto()) {
        content.height = height.resolve(font_size, root_font_size_);
    }

    if (auto min = box.get_property<css::PropertyId::MinHeight>(); !min.is_auto()) {
        content.height = std::max(content.height, min.resolve(font_size, root_font_size_));
    }

    if (auto max = box.get_property<css::PropertyId::MaxHeight>(); !max.is_none()) {
        content.height = std::min(content.height, max.resolve(font_size, root_font_size_));
    }
}

void Layouter::calculate_padding(LayoutBox &box, int const font_size) const {
    auto &padding = box.dimensions.padding;
    padding.left = box.get_property<css::PropertyId::PaddingLeft>().resolve(font_size, root_font_size_);
    padding.right = box.get_property<css::PropertyId::PaddingRight>().resolve(font_size, root_font_size_);
    padding.top = box.get_property<css::PropertyId::PaddingTop>().resolve(font_size, root_font_size_);
    padding.bottom = box.get_property<css::PropertyId::PaddingBottom>().resolve(font_size, root_font_size_);
}

void Layouter::calculate_border(LayoutBox &box, int const font_size) const {
    if (box.get_property<css::PropertyId::BorderLeftStyle>() != style::BorderStyle::None) {
        auto border_width = box.get_property<css::PropertyId::BorderLeftWidth>();
        box.dimensions.border.left = border_width.resolve(font_size, root_font_size_);
    }

    if (box.get_property<css::PropertyId::BorderRightStyle>() != style::BorderStyle::None) {
        auto border_width = box.get_property<css::PropertyId::BorderRightWidth>();
        box.dimensions.border.right = border_width.resolve(font_size, root_font_size_);
    }

    if (box.get_property<css::PropertyId::BorderTopStyle>() != style::BorderStyle::None) {
        auto border_width = box.get_property<css::PropertyId::BorderTopWidth>();
        box.dimensions.border.top = border_width.resolve(font_size, root_font_size_);
    }

    if (box.get_property<css::PropertyId::BorderBottomStyle>() != style::BorderStyle::None) {
        auto border_width = box.get_property<css::PropertyId::BorderBottomWidth>();
        box.dimensions.border.bottom = border_width.resolve(font_size, root_font_size_);
    }
}

std::optional<std::shared_ptr<type::IFont const>> Layouter::find_font(
        std::span<std::string_view const> font_families) const {
    for (auto const &family : font_families) {
        if (auto font = type_.font(family)) {
            return font;
        }
    }
    return std::nullopt;
}

} // namespace

std::optional<LayoutBox> create_layout(style::StyledNode const &node, int width, type::IType const &type) {
    auto tree = create_tree(node);
    if (!tree) {
        return {};
    }

    // TODO(robinlinden): Merge the different passes. They're separate because
    // it was easier, but it's definitely less efficient.
    collapse_whitespace(*tree);
    apply_text_transforms(*tree);

    Layouter{node.get_property<css::PropertyId::FontSize>(), type}.layout(*tree, {0, 0, width, 0});
    return tree;
}

} // namespace layout
