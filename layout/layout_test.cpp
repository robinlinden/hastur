#include "layout/layout.h"

#include "etest/etest.h"

using namespace std::literals;
using etest::expect;
using etest::require;
using layout::LayoutType;

int main() {
    etest::test("simple tree", [] {
        auto dom_root = dom::create_element_node("html", {}, {
            dom::create_element_node("head", {}, {}),
            dom::create_element_node("body", {}, {
                dom::create_element_node("p", {}, {}),
            }),
        });

        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {},
            .children = {
                {dom_root.children[0], {}, {}},
                {dom_root.children[1], {}, {
                    {dom_root.children[1].children[0], {}, {}},
                }},
            },
        };

        auto expected_layout = layout::LayoutBox{
            .node = &style_root,
            .type = LayoutType::Block,
            .dimensions = {},
            .children = {
                {&style_root.children[0], LayoutType::Block, {}, {}},
                {&style_root.children[1], LayoutType::Block, {}, {
                    {&style_root.children[1].children[0], LayoutType::Block, {}, {}},
                }},
            }
        };

        auto layout_root = layout::create_layout(style_root);
        expect(expected_layout == layout_root);
    });

    etest::test("layouting removes display:none nodes", [] {
        auto dom_root = dom::create_element_node("html", {}, {
            dom::create_element_node("head", {}, {}),
            dom::create_element_node("body", {}, {
                dom::create_element_node("p", {}, {}),
            }),
        });

        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {},
            .children = {
                {dom_root.children[0], {{"display", "none"}}, {}},
                {dom_root.children[1], {}, {
                    {dom_root.children[1].children[0], {}, {}},
                }},
            },
        };

        auto expected_layout = layout::LayoutBox{
            .node = &style_root,
            .type = LayoutType::Block,
            .dimensions = {},
            .children = {
                {&style_root.children[1], LayoutType::Block, {}, {
                    {&style_root.children[1].children[0], LayoutType::Block, {}, {}},
                }},
            }
        };

        auto layout_root = layout::create_layout(style_root);
        expect(expected_layout == layout_root);
    });

    etest::test("inline nodes get wrapped in anonymous blocks", [] {
        auto dom_root = dom::create_element_node("html", {}, {
            dom::create_element_node("head", {}, {}),
            dom::create_element_node("body", {}, {
                dom::create_element_node("p", {}, {}),
            }),
        });

        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {},
            .children = {
                {dom_root.children[0], {{"display", "inline"}}, {}},
                {dom_root.children[1], {{"display", "inline"}}, {
                    {dom_root.children[1].children[0], {}, {}},
                }},
            },
        };

        // TODO(robinlinden)
        // Having block elements inside of inline ones isn't allowed,
        // but I haven't looked up how to handle them yet.
        auto expected_layout = layout::LayoutBox{
            .node = &style_root,
            .type = LayoutType::Block,
            .dimensions = {},
            .children = {
                {nullptr, LayoutType::AnonymousBlock, {}, {
                    {&style_root.children[0], LayoutType::Inline, {}, {}},
                    {&style_root.children[1], LayoutType::Inline, {}, {
                        {&style_root.children[1].children[0], LayoutType::Block, {}, {}},
                    }},
                }},
            }
        };

        auto layout_root = layout::create_layout(style_root);
        expect(expected_layout == layout_root);
    });

    etest::test("text", [] {
        auto dom_root = dom::create_element_node("html", {}, {
            dom::create_element_node("body", {}, {
                dom::create_text_node("hello"),
                dom::create_text_node("goodbye"),
            }),
        });

        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {},
            .children = {
                {dom_root.children[0], {}, {
                    {dom_root.children[0].children[0], {}, {}},
                    {dom_root.children[0].children[1], {}, {}},
                }},
            },
        };

        auto expected_layout = layout::LayoutBox{
            .node = &style_root,
            .type = LayoutType::Block,
            .dimensions = {},
            .children = {
                {&style_root.children[0], LayoutType::Block, {}, {
                    {nullptr, LayoutType::AnonymousBlock, {}, {
                        {&style_root.children[0].children[0], LayoutType::Inline, {}, {}},
                        {&style_root.children[0].children[1], LayoutType::Inline, {}, {}},
                    }},
                }},
            }
        };

        auto layout_root = layout::create_layout(style_root);
        expect(expected_layout == layout_root);
    });

    return etest::run_all_tests();
}
