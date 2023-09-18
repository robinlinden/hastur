// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "layout/layout_box.h"

#include "layout/layout.h"

#include "dom/dom.h"
#include "etest/etest.h"

#include <string_view>
#include <utility>
#include <vector>

using etest::expect;
using etest::expect_eq;
using layout::LayoutType;
using namespace std::literals;

namespace {

// Until we have a nicer tree-creation abstraction for the tests, this needs to
// be called if a test relies on property inheritance.
void set_up_parent_ptrs(style::StyledNode &parent) {
    for (auto &child : parent.children) {
        child.parent = &parent;
        set_up_parent_ptrs(child);
    }
}

} // namespace

int main() {
    etest::test("text", [] {
        dom::Node dom_root =
                dom::Element{"html", {}, {dom::Element{"body", {}, {dom::Text{"hello"}, dom::Text{"goodbye"}}}}};

        auto const &children = std::get<dom::Element>(dom_root).children;

        std::vector<style::StyledNode> style_children{
                {std::get<dom::Element>(children[0]).children[0], {}, {}},
                {std::get<dom::Element>(children[0]).children[1], {}, {}},
        };
        auto style_root = style::StyledNode{
                .node = dom_root,
                .properties = {{css::PropertyId::Display, "block"}, {css::PropertyId::FontSize, "10px"}},
                .children{
                        {children[0], {{css::PropertyId::Display, "block"}}, {std::move(style_children)}},
                },
        };
        set_up_parent_ptrs(style_root);

        std::vector<layout::LayoutBox> layout_children{
                {&style_root.children[0].children[0], LayoutType::Inline, {{0, 0, 25, 10}}, {}, "hello"sv},
                {&style_root.children[0].children[1], LayoutType::Inline, {{25, 0, 35, 10}}, {}, "goodbye"sv},
        };
        auto expected_layout = layout::LayoutBox{.node = &style_root,
                .type = LayoutType::Block,
                .dimensions = {{0, 0, 0, 10}},
                .children{{
                        &style_root.children[0],
                        LayoutType::Block,
                        {{0, 0, 0, 10}},
                        {{nullptr, LayoutType::AnonymousBlock, {{0, 0, 60, 10}}, {std::move(layout_children)}}},
                }}};

        auto layout_root = layout::create_layout(style_root, 0);
        expect(expected_layout == layout_root);

        expect_eq(expected_layout.children.at(0).children.at(0).children.at(0).text(), "hello");
        expect_eq(expected_layout.children.at(0).children.at(0).children.at(1).text(), "goodbye");
    });

    etest::test("box_at_position", [] {
        std::vector<layout::LayoutBox> children{
                {nullptr, LayoutType::AnonymousBlock, {{30, 30, 5, 5}}, {}},
                {nullptr, LayoutType::Block, {{45, 45, 5, 5}}, {}},
        };

        auto layout = layout::LayoutBox{
                .node = nullptr,
                .type = LayoutType::Block,
                .dimensions = {{0, 0, 100, 100}},
                .children{
                        {nullptr, LayoutType::Block, {{25, 25, 50, 50}}, {std::move(children)}},
                },
        };

        expect(box_at_position(layout, {-1, -1}) == nullptr);
        expect(box_at_position(layout, {101, 101}) == nullptr);

        expect(box_at_position(layout, {100, 100}) == &layout);
        expect(box_at_position(layout, {0, 0}) == &layout);

        // We don't want to end up in anonymous blocks, so this should return its parent.
        expect(box_at_position(layout, {31, 31}) == &layout.children[0]);

        expect(box_at_position(layout, {75, 75}) == &layout.children[0]);
        expect(box_at_position(layout, {47, 47}) == &layout.children[0].children[1]);
    });

    etest::test("xpath", [] {
        dom::Node html_node = dom::Element{"html"s};
        dom::Node div_node = dom::Element{"div"s};
        dom::Node p_node = dom::Element{"p"s};
        dom::Node text_node = dom::Text{"hello!"s};
        style::StyledNode styled_node{
                .node = html_node,
                .properties{{css::PropertyId::Display, "block"}},
                .children{
                        style::StyledNode{.node = div_node, .properties{{css::PropertyId::Display, "block"}}},
                        style::StyledNode{.node = text_node},
                        style::StyledNode{.node = div_node,
                                .children{
                                        style::StyledNode{.node = p_node},
                                        style::StyledNode{.node = text_node},
                                }},
                },
        };
        auto layout = layout::create_layout(styled_node, 123).value();

        // Verify that we have a shady anon-box to deal with in here.
        expect_eq(layout.children.size(), std::size_t{2});
        expect_eq(layout.children.at(1).type, LayoutType::AnonymousBlock);

        auto const &anon_block = layout.children.at(1);

        using NodeVec = std::vector<layout::LayoutBox const *>;
        expect_eq(dom::nodes_by_xpath(layout, "/html"), NodeVec{&layout});
        expect_eq(dom::nodes_by_xpath(layout, "/html/div"), NodeVec{&layout.children[0], &anon_block.children[1]});
        expect_eq(dom::nodes_by_xpath(layout, "/html/div/"), NodeVec{});
        expect_eq(dom::nodes_by_xpath(layout, "/html/div/p"), NodeVec{&anon_block.children[1].children[0]});
        expect_eq(dom::nodes_by_xpath(layout, "/htm/div"), NodeVec{});
        expect_eq(dom::nodes_by_xpath(layout, "//div"), NodeVec{&layout.children[0], &anon_block.children[1]});
    });

    etest::test("to_string", [] {
        auto body = dom::Element{"body", {}, {dom::Element{"p", {}, {dom::Text{"!!!\n\n!!!"}}}, dom::Element{"p"}}};
        dom::Node dom_root = dom::Element{.name{"html"}, .children{std::move(body)}};

        auto const &html_children = std::get<dom::Element>(dom_root).children;
        auto const &body_children = std::get<dom::Element>(html_children[0]).children;

        auto text_child = style::StyledNode{std::get<dom::Element>(body_children[0]).children[0]};
        auto body_style_children = std::vector<style::StyledNode>{
                {
                        body_children[0],
                        {{css::PropertyId::Height, "25px"}, {css::PropertyId::Display, "block"}},
                        {std::move(text_child)},
                },
                {
                        body_children[1],
                        {{css::PropertyId::PaddingTop, "5px"},
                                {css::PropertyId::PaddingRight, "15px"},
                                {css::PropertyId::Display, "block"}},
                },
        };
        auto body_style = style::StyledNode{
                html_children[0],
                {{css::PropertyId::Width, "50px"}, {css::PropertyId::Display, "block"}},
                std::move(body_style_children),
        };
        auto style_root = style::StyledNode{
                .node = dom_root,
                .properties = {{css::PropertyId::Display, "block"}, {css::PropertyId::FontSize, "10px"}},
                .children{{std::move(body_style)}},
        };
        set_up_parent_ptrs(style_root);

        auto const *expected =
                "html\n"
                "block {0,0,0,30} {0,0,0,0} {0,0,0,0}\n"
                "  body\n"
                "  block {0,0,50,30} {0,0,0,0} {0,0,0,0}\n"
                "    p\n"
                "    block {0,0,50,25} {0,0,0,0} {0,0,0,0}\n"
                "      ablock {0,0,35,25} {0,0,0,0} {0,0,0,0}\n"
                "        !!! !!!\n"
                "        inline {0,0,35,25} {0,0,0,0} {0,0,0,0}\n"
                "    p\n"
                "    block {0,30,35,0} {5,15,0,0} {0,0,0,0}\n";
        expect_eq(to_string(layout::create_layout(style_root, 0).value()), expected);
    });

    return etest::run_all_tests();
}
