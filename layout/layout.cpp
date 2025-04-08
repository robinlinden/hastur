// SPDX-FileCopyrightText: 2021-2025 Robin Lindén <dev@robinlinden.eu>
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

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <functional>
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
    Layouter(style::ResolutionInfo context,
            type::IType const &type,
            std::function<std::optional<Size>(std::string_view)> const &get_intrensic_size_for_resource_at_url)
        : resolution_context_{context}, type_{type},
          get_intrensic_size_for_resource_at_url_{get_intrensic_size_for_resource_at_url} {}

    void layout(LayoutBox &, geom::Rect const &bounds, int last_block_width) const;

private:
    style::ResolutionInfo resolution_context_;
    type::IType const &type_;
    std::function<std::optional<Size>(std::string_view)> get_intrensic_size_for_resource_at_url_;

    void layout_inline(LayoutBox &, geom::Rect const &bounds, int last_block_width) const;
    void layout_block(LayoutBox &, geom::Rect const &bounds, int last_block_width) const;
    void layout_anonymous_block(LayoutBox &, geom::Rect const &bounds, int last_block_width) const;

    void calculate_left_and_right_margin(LayoutBox &,
            int parent_width,
            style::UnresolvedValue margin_left,
            style::UnresolvedValue margin_right,
            int font_size) const;
    void calculate_width_and_margin(LayoutBox &, geom::Rect const &parent, int font_size, int last_block_width) const;
    void calculate_inline_height(LayoutBox &, int font_size) const;
    void calculate_non_inline_height(LayoutBox &, int font_size) const;
    void calculate_padding(LayoutBox &, int font_size) const;
    void calculate_border(LayoutBox &, int font_size) const;
    std::optional<std::shared_ptr<type::IFont const>> find_font(std::span<std::string_view const> font_families) const;
};

bool last_node_was_anonymous(LayoutBox const &box) {
    return !box.children.empty() && box.children.back().is_anonymous_block();
}

// https://www.w3.org/TR/CSS2/visuren.html#box-gen
// NOLINTNEXTLINE(misc-no-recursion)
std::optional<LayoutBox> create_tree(
        style::StyledNode const &node, std::function<bool(std::string_view)> const &resource_exists) {
    if (auto const *text = std::get_if<dom::Text>(&node.node)) {
        return LayoutBox{.node = &node, .layout_text = std::string_view{text->text}};
    }

    assert(std::holds_alternative<dom::Element>(node.node));
    auto display = node.get_property<css::PropertyId::Display>();
    if (!display.has_value()) {
        return std::nullopt;
    }

    if (auto const &element = std::get<dom::Element>(node.node); element.name == "img"sv) {
        auto src = element.attributes.find("src"sv);
        if (src == element.attributes.end() || !resource_exists(src->second)) {
            if (auto alt = element.attributes.find("alt"sv); alt != element.attributes.end()) {
                return LayoutBox{.node = &node, .layout_text = std::string_view{alt->second}};
            }
        }
    }

    LayoutBox box{&node};

    for (auto const &child : node.children) {
        auto child_box = create_tree(child, resource_exists);
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

    // TODO(robinlinden): More accurate handling of the white-space property.
    auto starts_text_run = [](LayoutBox const &l) {
        return !std::holds_alternative<std::monostate>(l.layout_text)
                && l.get_property<css::PropertyId::WhiteSpace>() == style::WhiteSpace::Normal;
    };
    auto ends_text_run = [](LayoutBox const &l) {
        return l.is_anonymous_block() || l.get_property<css::PropertyId::Display>() != style::Display::inline_flow()
                || l.get_property<css::PropertyId::WhiteSpace>() != style::WhiteSpace::Normal;
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
        } else if (last_text_box != nullptr
                && (!std::holds_alternative<std::monostate>(current.layout_text)
                        && current.get_property<css::PropertyId::WhiteSpace>() == style::WhiteSpace::Normal)) {
            // Remove all but 1 trailing space.
            auto &last_text = std::get<std::string_view>(last_text_box->layout_text);
            auto last_non_whitespace_idx = last_text.find_last_not_of(" \n\r\f\v\t");
            if (last_non_whitespace_idx != std::string_view::npos) {
                auto trailing_whitespace_count = last_text.size() - last_non_whitespace_idx - 1;
                if (trailing_whitespace_count > 1) {
                    last_text.remove_suffix(trailing_whitespace_count - 1);
                }
            }

            if (last_text.empty() || util::is_whitespace(last_text.back())) {
                current.layout_text = util::trim_start(std::get<std::string_view>(current.layout_text));
            }

            if (needs_allocating_whitespace_collapsing(last_text)) {
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
void Layouter::layout(LayoutBox &box, geom::Rect const &bounds, int last_block_width) const {
    if (box.is_anonymous_block()) {
        layout_anonymous_block(box, bounds, last_block_width);
        return;
    }

    // Nodes w/ `display: none` aren't added to the tree and shouldn't end up here.
    auto display = box.get_property<css::PropertyId::Display>();
    assert(display.has_value());
    if (display == style::Display::inline_flow()) {
        layout_inline(box, bounds, last_block_width);
        return;
    }

    layout_block(box, bounds, last_block_width);
}

type::Weight to_type(std::optional<style::FontWeight> const &weight) {
    if (!weight || weight->value < style::FontWeight::kBold) {
        return type::Weight::Normal;
    }

    return type::Weight::Bold;
}

std::string_view try_get_src(LayoutBox const &box) {
    assert(!box.is_anonymous_block());
    auto const *img = std::get_if<dom::Element>(&box.node->node);
    if (img == nullptr || img->name != "img"sv) {
        return {};
    }

    auto src = img->attributes.find("src"sv);
    if (src == img->attributes.end()) {
        return {};
    }

    return src->second;
}

// NOLINTNEXTLINE(misc-no-recursion)
void Layouter::layout_inline(LayoutBox &box, geom::Rect const &bounds, int last_block_width) const {
    assert(box.node);
    auto font_size = box.get_property<css::PropertyId::FontSize>();
    calculate_padding(box, font_size);
    calculate_border(box, font_size);

    if (auto text = box.text()) {
        assert(box.children.empty());
        auto font_families = box.get_property<css::PropertyId::FontFamily>();
        auto weight = to_type(box.get_property<css::PropertyId::FontWeight>());
        auto font = find_font(font_families);
        if (font) {
            box.dimensions.content.width = (*font)->measure(*text, type::Px{font_size}, weight).width;
        } else {
            spdlog::warn("No font found for font-families: {}", util::join(font_families, ", "));
            box.dimensions.content.width = type::NaiveFont{}.measure(*text, type::Px{font_size}, weight).width;
        }
    } else if (auto src = try_get_src(box); !src.empty()) {
        // https://www.w3.org/TR/CSS22/visudet.html#inline-replaced-width
        // TODO(robinlinden): Apply things like max-{width,height}.
        assert(box.children.empty()); // <img> is a void element.
        if (auto maybe_size = get_intrensic_size_for_resource_at_url_(src); maybe_size.has_value()) {
            box.dimensions.content.width = maybe_size->width;
            box.dimensions.content.height = maybe_size->height;
        }
    } else {
        // When attempting to wrap inline content, we sometimes try multiple layout
        // candidates, so width += <...> without resetting it between runs is bad.
        box.dimensions.content.width = 0;
    }

    if (box.node->parent != nullptr) {
        auto const &d = box.dimensions;
        box.dimensions.content.x = bounds.x + d.padding.left + d.border.left + d.margin.left;
        box.dimensions.content.y = bounds.y + d.border.top + d.padding.top + d.margin.top;
    }

    int last_child_end{};
    for (auto &child : box.children) {
        layout(child, box.dimensions.content.translated(last_child_end, 0), last_block_width);
        last_child_end += child.dimensions.margin_box().width;
        box.dimensions.content.height = std::max(box.dimensions.content.height, child.dimensions.margin_box().height);
        box.dimensions.content.width += child.dimensions.margin_box().width;
    }
    calculate_inline_height(box, font_size);
}

// NOLINTNEXTLINE(misc-no-recursion)
void Layouter::layout_block(LayoutBox &box, geom::Rect const &bounds, int last_block_width) const {
    assert(box.node);
    auto font_size = box.get_property<css::PropertyId::FontSize>();
    calculate_padding(box, font_size);
    calculate_border(box, font_size);
    calculate_width_and_margin(box, bounds, font_size, last_block_width);
    calculate_position(box, bounds);
    for (auto &child : box.children) {
        layout(child, box.dimensions.content, box.dimensions.content.width);
        box.dimensions.content.height += child.dimensions.margin_box().height;
    }
    calculate_non_inline_height(box, font_size);
}

// NOLINTNEXTLINE(misc-no-recursion)
void Layouter::layout_anonymous_block(LayoutBox &box, geom::Rect const &bounds, int last_block_width) const {
    calculate_position(box, bounds);
    box.dimensions.content.width = last_block_width;
    int last_child_end{};
    int current_line{};
    auto font_size = type::Px{box.get_property<css::PropertyId::FontSize>()};
    auto font_families = box.get_property<css::PropertyId::FontFamily>();

    auto maybe_font = find_font(font_families);
    if (!maybe_font) {
        spdlog::warn("No font found for font-families: {}", util::join(font_families, ", "));
        maybe_font = std::make_shared<type::NaiveFont>();
    }
    auto font = *maybe_font;

    auto weight = to_type(box.get_property<css::PropertyId::FontWeight>());

    for (std::size_t i = 0; i < box.children.size(); ++i) {
        auto *child = &box.children[i];
        layout(*child, box.dimensions.content.translated(last_child_end, current_line * font_size.v), last_block_width);

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
            // Is the entire box out-of-bounds?
            if (child->dimensions.margin_box().x - box.dimensions.margin_box().x > bounds.width) {
                last_child_end = 0;
                current_line += 1;
                layout(*child, box.dimensions.content.translated(0, current_line * font_size.v), last_block_width);
                continue;
            }

            // Does the box contain splittable text?
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
                    last_child_end += child->dimensions.margin_box().width;
                }
            }
        } else {
            last_child_end += child->dimensions.margin_box().width;
        }
    }

    assert(!box.children.empty());
    auto last_size = box.children.back().dimensions.margin_box();
    box.dimensions.content.height = last_size.y - box.dimensions.margin_box().y + last_size.height;
}

void Layouter::calculate_left_and_right_margin(LayoutBox &box,
        int const parent_width,
        style::UnresolvedValue margin_left,
        style::UnresolvedValue margin_right,
        int const font_size) const {
    if (margin_left.is_auto() && margin_right.is_auto()) {
        int margin_px = (parent_width - box.dimensions.border_box().width) / 2;
        box.dimensions.margin.left = box.dimensions.margin.right = margin_px;
    } else if (margin_left.is_auto() && !margin_right.is_auto()) {
        box.dimensions.margin.right = margin_right.resolve(font_size, resolution_context_);
        box.dimensions.margin.left = parent_width - box.dimensions.margin_box().width;
    } else if (!margin_left.is_auto() && margin_right.is_auto()) {
        box.dimensions.margin.left = margin_left.resolve(font_size, resolution_context_);
        box.dimensions.margin.right = parent_width - box.dimensions.margin_box().width;
    } else {
        // TODO(mkiael): Compute margin depending on direction property
    }
}

// https://www.w3.org/TR/CSS2/visudet.html#blockwidth
void Layouter::calculate_width_and_margin(
        LayoutBox &box, geom::Rect const &parent, int const font_size, int const last_block_width) const {
    assert(box.node != nullptr);

    auto &margins = box.dimensions.margin;
    if (auto margin_top = box.get_property<css::PropertyId::MarginTop>(); !margin_top.is_auto()) {
        margins.top = margin_top.resolve(font_size, resolution_context_);
    } else {
        margins.top = 0;
    }

    if (auto margin_bottom = box.get_property<css::PropertyId::MarginBottom>(); !margin_bottom.is_auto()) {
        margins.bottom = margin_bottom.resolve(font_size, resolution_context_);
    } else {
        margins.bottom = 0;
    }

    auto margin_left = box.get_property<css::PropertyId::MarginLeft>();
    auto margin_right = box.get_property<css::PropertyId::MarginRight>();
    auto width = box.get_property<css::PropertyId::Width>();
    std::optional<int> resolved_width;
    if (!width.is_auto()) {
        resolved_width = width.try_resolve(font_size, resolution_context_, last_block_width);
    }

    if (resolved_width) {
        box.dimensions.content.width = *resolved_width;
        calculate_left_and_right_margin(box, last_block_width, margin_left, margin_right, font_size);
    } else {
        if (!margin_left.is_auto()) {
            margins.left = margin_left.resolve(font_size, resolution_context_);
        }
        if (!margin_right.is_auto()) {
            margins.right = margin_right.resolve(font_size, resolution_context_);
        }
        box.dimensions.content.width = last_block_width - box.dimensions.margin_box().width;
    }

    if (auto min = box.get_property<css::PropertyId::MinWidth>(); !min.is_auto()) {
        auto resolved = min.resolve(font_size, resolution_context_, last_block_width);
        if (box.dimensions.content.width < resolved) {
            box.dimensions.content.width = resolved;
            calculate_left_and_right_margin(box, last_block_width, margin_left, margin_right, font_size);
        }
    }

    auto max = box.get_property<css::PropertyId::MaxWidth>();
    std::optional<int> resolved_max;
    if (!max.is_none()) {
        resolved_max = max.try_resolve(font_size, resolution_context_, parent.width);
    }

    if (resolved_max) {
        if (box.dimensions.content.width > *resolved_max) {
            box.dimensions.content.width = *resolved_max;
            calculate_left_and_right_margin(box, last_block_width, margin_left, margin_right, font_size);
        }
    }
}

void Layouter::calculate_inline_height(LayoutBox &box, int const font_size) const {
    assert(box.node != nullptr);
    if (auto text = box.text()) {
        int lines = static_cast<int>(std::ranges::count(*text, '\n')) + 1;
        box.dimensions.content.height = lines * font_size;
    }
}

void Layouter::calculate_non_inline_height(LayoutBox &box, int const font_size) const {
    assert(box.node != nullptr);
    auto &content = box.dimensions.content;

    // TODO(robinlinden): Handling text here might not be required.
    if (auto text = box.text()) {
        int lines = static_cast<int>(std::ranges::count(*text, '\n')) + 1;
        content.height = lines * font_size;
    }

    if (auto height = box.get_property<css::PropertyId::Height>(); !height.is_auto()) {
        if (box.node->parent == nullptr) {
            content.height = height.resolve(font_size, resolution_context_, resolution_context_.viewport_height);
        } else if (auto maybe_height = height.try_resolve(font_size, resolution_context_)) {
            content.height = *maybe_height;
        }
    }

    if (auto min = box.get_property<css::PropertyId::MinHeight>(); !min.is_auto()) {
        content.height = std::max(content.height, min.resolve(font_size, resolution_context_));
    }

    if (auto max = box.get_property<css::PropertyId::MaxHeight>(); !max.is_none()) {
        content.height = std::min(content.height, max.resolve(font_size, resolution_context_));
    }
}

void Layouter::calculate_padding(LayoutBox &box, int const font_size) const {
    auto &padding = box.dimensions.padding;
    padding.left = box.get_property<css::PropertyId::PaddingLeft>().resolve(font_size, resolution_context_);
    padding.right = box.get_property<css::PropertyId::PaddingRight>().resolve(font_size, resolution_context_);
    padding.top = box.get_property<css::PropertyId::PaddingTop>().resolve(font_size, resolution_context_);
    padding.bottom = box.get_property<css::PropertyId::PaddingBottom>().resolve(font_size, resolution_context_);
}

void Layouter::calculate_border(LayoutBox &box, int const font_size) const {
    if (box.get_property<css::PropertyId::BorderLeftStyle>() != style::BorderStyle::None) {
        auto border_width = box.get_property<css::PropertyId::BorderLeftWidth>();
        box.dimensions.border.left = border_width.resolve(font_size, resolution_context_);
    }

    if (box.get_property<css::PropertyId::BorderRightStyle>() != style::BorderStyle::None) {
        auto border_width = box.get_property<css::PropertyId::BorderRightWidth>();
        box.dimensions.border.right = border_width.resolve(font_size, resolution_context_);
    }

    if (box.get_property<css::PropertyId::BorderTopStyle>() != style::BorderStyle::None) {
        auto border_width = box.get_property<css::PropertyId::BorderTopWidth>();
        box.dimensions.border.top = border_width.resolve(font_size, resolution_context_);
    }

    if (box.get_property<css::PropertyId::BorderBottomStyle>() != style::BorderStyle::None) {
        auto border_width = box.get_property<css::PropertyId::BorderBottomWidth>();
        box.dimensions.border.bottom = border_width.resolve(font_size, resolution_context_);
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

std::optional<LayoutBox> create_layout(style::StyledNode const &node,
        LayoutInfo const &info,
        type::IType const &type,
        std::function<std::optional<Size>(std::string_view)> const &get_intrensic_size_for_resource_at_url) {
    auto resource_exists = [&get_intrensic_size_for_resource_at_url](std::string_view url) {
        return get_intrensic_size_for_resource_at_url(url).has_value();
    };

    auto tree = create_tree(node, resource_exists);
    if (!tree) {
        return {};
    }

    // TODO(robinlinden): Merge the different passes. They're separate because
    // it was easier, but it's definitely less efficient.
    collapse_whitespace(*tree);
    apply_text_transforms(*tree);

    style::ResolutionInfo resolution_context{
            .root_font_size = node.get_property<css::PropertyId::FontSize>(),
            .viewport_width = info.viewport_width,
            .viewport_height = info.viewport_height,
    };

    Layouter{resolution_context, type, get_intrensic_size_for_resource_at_url}.layout(
            *tree, {0, 0, info.viewport_width, 0}, info.viewport_width);
    return tree;
}

} // namespace layout
