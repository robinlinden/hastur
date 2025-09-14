// SPDX-FileCopyrightText: 2021-2025 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "layout/layout_box.h"

#include "layout/layout.h"

#include "css/property_id.h"
#include "dom/dom.h"
#include "dom/xpath.h"
#include "etest/etest2.h"
#include "style/styled_node.h"
#include "style/unresolved_value.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace std::literals;

namespace {

// Until we have a nicer tree-creation abstraction for the tests, this needs to
// be called if a test relies on property inheritance.
void set_up_parent_ptrs(style::StyledNode &root) {
    std::vector<style::StyledNode *> stack{&root};
    while (!stack.empty()) {
        auto *node = stack.back();
        stack.pop_back();
        for (auto &child : node->children) {
            child.parent = node;
            stack.push_back(&child);
        }
    }
}

} // namespace

int main() {
    etest::Suite s{};
    s.add_test("text", [](etest::IActions &a) {
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
                {&style_root.children[0].children[0], {{0, 0, 25, 10}}, {}, "hello"sv},
                {&style_root.children[0].children[1], {{25, 0, 35, 10}}, {}, "goodbye"sv},
        };
        auto expected_layout = layout::LayoutBox{.node = &style_root,
                .dimensions = {{0, 0, 100, 10}},
                .children{{
                        &style_root.children[0],
                        {{0, 0, 100, 10}},
                        {{nullptr, {{0, 0, 100, 10}}, {std::move(layout_children)}}},
                }}};

        auto layout_root = layout::create_layout(style_root, {.viewport_width = 100});
        a.expect(expected_layout == layout_root);

        a.expect_eq(expected_layout.children.at(0).children.at(0).children.at(0).text(), "hello");
        a.expect_eq(expected_layout.children.at(0).children.at(0).children.at(1).text(), "goodbye");
    });

    s.add_test("box_at_position", [](etest::IActions &a) {
        dom::Node dom = dom::Element{"dummy"};
        style::StyledNode style{dom, {{css::PropertyId::Display, "block"}}};
        std::vector<layout::LayoutBox> children{
                {nullptr, {{30, 30, 5, 5}}, {}},
                {&style, {{45, 45, 5, 5}}, {}},
        };

        auto layout = layout::LayoutBox{
                .node = &style,
                .dimensions = {{0, 0, 100, 100}},
                .children{
                        {&style, {{25, 25, 50, 50}}, {std::move(children)}},
                },
        };

        a.expect(box_at_position(layout, {-1, -1}) == nullptr);
        a.expect(box_at_position(layout, {101, 101}) == nullptr);

        a.expect(box_at_position(layout, {100, 100}) == &layout);
        a.expect(box_at_position(layout, {0, 0}) == &layout);

        // We don't want to end up in anonymous blocks, so this should return its parent.
        a.expect(box_at_position(layout, {31, 31}) == &layout.children[0]);

        a.expect(box_at_position(layout, {75, 75}) == &layout.children[0]);
        a.expect(box_at_position(layout, {47, 47}) == &layout.children[0].children[1]);
    });

    s.add_test("xpath", [](etest::IActions &a) {
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

        set_up_parent_ptrs(styled_node);

        auto layout = layout::create_layout(styled_node, {.viewport_width = 123}).value();

        // Verify that we have a shady anon-box to deal with in here.
        a.expect_eq(layout.children.size(), std::size_t{2});

        auto const &anon_block = layout.children.at(1);

        using NodeVec = std::vector<layout::LayoutBox const *>;
        a.expect_eq(dom::nodes_by_xpath(layout, "/html"), NodeVec{&layout});
        a.expect_eq(dom::nodes_by_xpath(layout, "/html/div"), NodeVec{&layout.children[0], &anon_block.children[1]});
        a.expect_eq(dom::nodes_by_xpath(layout, "/html/div/"), NodeVec{});
        a.expect_eq(dom::nodes_by_xpath(layout, "/html/div/p"), NodeVec{&anon_block.children[1].children[0]});
        a.expect_eq(dom::nodes_by_xpath(layout, "/htm/div"), NodeVec{});
        a.expect_eq(dom::nodes_by_xpath(layout, "//div"), NodeVec{&layout.children[0], &anon_block.children[1]});
    });

    s.add_test("to_string", [](etest::IActions &a) {
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
                "      ablock {0,0,50,10} {0,0,0,0} {0,0,0,0}\n"
                "        !!! !!!\n"
                "        inline {0,0,35,10} {0,0,0,0} {0,0,0,0}\n"
                "    p\n"
                "    block {0,30,35,0} {5,15,0,0} {0,0,0,0}\n";
        a.expect_eq(to_string(layout::create_layout(style_root, {.viewport_width = 0}).value()), expected);
    });

    s.add_test("anonymous block, get_property", [](etest::IActions &a) {
        a.expect_eq(layout::LayoutBox{}.get_property<css::PropertyId::Width>(), style::UnresolvedValue{"auto"});
    });

    return s.run();
}
