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

        auto layout_root = layout::create_layout(style_root, 0);
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

        auto layout_root = layout::create_layout(style_root, 0);
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

        auto layout_root = layout::create_layout(style_root, 0);
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

        auto layout_root = layout::create_layout(style_root, 0);
        expect(expected_layout == layout_root);
    });

    etest::test("simple width", [] {
        auto dom_root = dom::create_element_node("html", {}, {
            dom::create_element_node("body", {}, {
                dom::create_element_node("p", {}, {}),
            }),
        });

        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {{"width", "100px"}},
            .children = {
                {dom_root.children[0], {}, {
                    {dom_root.children[0].children[0], {}, {}},
                }},
            },
        };

        auto expected_layout = layout::LayoutBox{
            .node = &style_root,
            .type = LayoutType::Block,
            .dimensions = {{0, 0, 100, 0}},
            .children = {
                {&style_root.children[0], LayoutType::Block, {{0, 0, 100, 0}}, {
                    {&style_root.children[0].children[0], LayoutType::Block, {{0, 0, 100, 0}}, {}},
                }},
            }
        };

        expect(layout::create_layout(style_root, 1000) == expected_layout);
    });

    etest::test("less simple width", [] {
        auto dom_root = dom::create_element_node("html", {}, {
            dom::create_element_node("body", {}, {
                dom::create_element_node("p", {}, {}),
            }),
        });

        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {{"width", "100px"}},
            .children = {
                {dom_root.children[0], {{"width", "50px"}}, {
                    {dom_root.children[0].children[0], {{"width", "25px"}}, {}},
                }},
            },
        };

        auto expected_layout = layout::LayoutBox{
            .node = &style_root,
            .type = LayoutType::Block,
            .dimensions = {{0, 0, 100, 0}},
            .children = {
                {&style_root.children[0], LayoutType::Block, {{0, 0, 50, 0}}, {
                    {&style_root.children[0].children[0], LayoutType::Block, {{0, 0, 25, 0}}, {}},
                }},
            }
        };

        expect(layout::create_layout(style_root, 1000) == expected_layout);
    });

    etest::test("auto width expands to fill parent", [] {
        auto dom_root = dom::create_element_node("html", {}, {
            dom::create_element_node("body", {}, {
                dom::create_element_node("p", {}, {}),
            }),
        });

        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {{"width", "100px"}},
            .children = {
                {dom_root.children[0], {}, {
                    {dom_root.children[0].children[0], {}, {}},
                }},
            },
        };

        auto expected_layout = layout::LayoutBox{
            .node = &style_root,
            .type = LayoutType::Block,
            .dimensions = {{0, 0, 100, 0}},
            .children = {
                {&style_root.children[0], LayoutType::Block, {{0, 0, 100, 0}}, {
                    {&style_root.children[0].children[0], LayoutType::Block, {{0, 0, 100, 0}}, {}},
                }},
            }
        };

        expect(layout::create_layout(style_root, 1000) == expected_layout);
    });

    etest::test("height doesn't affect children", [] {
        auto dom_root = dom::create_element_node("html", {}, {
            dom::create_element_node("body", {}, {
                dom::create_element_node("p", {}, {}),
            }),
        });

        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {{"height", "100px"}},
            .children = {
                {dom_root.children[0], {}, {
                    {dom_root.children[0].children[0], {}, {}},
                }},
            },
        };

        auto expected_layout = layout::LayoutBox{
            .node = &style_root,
            .type = LayoutType::Block,
            .dimensions = {{0, 0, 0, 100}},
            .children = {
                {&style_root.children[0], LayoutType::Block, {{0, 0, 0, 0}}, {
                    {&style_root.children[0].children[0], LayoutType::Block, {{0, 0, 0, 0}}, {}},
                }},
            }
        };

        expect(layout::create_layout(style_root, 0) == expected_layout);
    });

    etest::test("height affects siblings and parents", [] {
        auto dom_root = dom::create_element_node("html", {}, {
            dom::create_element_node("body", {}, {
                dom::create_element_node("p", {}, {}),
                dom::create_element_node("p", {}, {}),
            }),
        });

        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {},
            .children = {
                {dom_root.children[0], {}, {
                    {dom_root.children[0].children[0], {{"height", "25px"}}, {}},
                    {dom_root.children[0].children[1], {}, {}},
                }},
            },
        };

        auto expected_layout = layout::LayoutBox{
            .node = &style_root,
            .type = LayoutType::Block,
            .dimensions = {{0, 0, 0, 25}},
            .children = {
                {&style_root.children[0], LayoutType::Block, {{0, 0, 0, 25}}, {
                    {&style_root.children[0].children[0], LayoutType::Block, {{0, 0, 0, 25}}, {}},
                    {&style_root.children[0].children[1], LayoutType::Block, {{0, 25, 0, 0}}, {}},
                }},
            }
        };

        expect(layout::create_layout(style_root, 0) == expected_layout);
    });

    etest::test("to_string", [] {
        auto dom_root = dom::create_element_node("html", {}, {
            dom::create_element_node("body", {}, {
                dom::create_element_node("p", {}, {}),
                dom::create_element_node("p", {}, {}),
            }),
        });

        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {},
            .children = {
                {dom_root.children[0], {}, {
                    {dom_root.children[0].children[0], {{"height", "25px"}}, {}},
                    {dom_root.children[0].children[1], {}, {}},
                }},
            },
        };

        auto expected =
                "html\n"
                "block {0,0,0,25}\n"
                "  body\n"
                "  block {0,0,0,25}\n"
                "    p\n"
                "    block {0,0,0,25}\n"
                "    p\n"
                "    block {0,25,0,0}\n";
        expect(to_string(layout::create_layout(style_root, 0)) == expected);
    });

    return etest::run_all_tests();
}
