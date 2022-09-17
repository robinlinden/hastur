// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "layout/layout.h"

#include "etest/etest.h"

using namespace std::literals;
using etest::expect;
using etest::expect_eq;
using etest::require;
using layout::LayoutType;

// TODO(robinlinden): clang-format doesn't get along well with how I structured
// the trees in these test cases.

// clang-format off

int main() {
    etest::test("simple tree", [] {
        auto dom_root = dom::create_element_node("html", {}, {
            dom::create_element_node("head", {}, {}),
            dom::create_element_node("body", {}, {
                dom::create_element_node("p", {}, {}),
            }),
        });

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {},
            .children = {
                {children[0], {}, {}},
                {children[1], {}, {
                    {std::get<dom::Element>(children[1]).children[0], {}, {}},
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

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {},
            .children = {
                {children[0], {{"display", "none"}}, {}},
                {children[1], {}, {
                    {std::get<dom::Element>(children[1]).children[0], {}, {}},
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

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {},
            .children = {
                {children[0], {{"display", "inline"}}, {}},
                {children[1], {{"display", "inline"}}, {
                    {std::get<dom::Element>(children[1]).children[0], {}, {}},
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

    // clang-format on
    etest::test("inline in inline don't get wrapped in anon-blocks", [] {
        auto dom_root = dom::create_element_node("span", {}, {dom::create_element_node("span", {}, {})});

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
                .node = dom_root,
                .properties = {{"display", "inline"}},
                .children = {{children[0], {{"display", "inline"}}, {}}},
        };

        auto expected_layout = layout::LayoutBox{
                .node = &style_root,
                .type = LayoutType::Inline,
                .dimensions = {},
                .children = {{&style_root.children[0], LayoutType::Inline, {}, {}}},
        };

        auto layout_root = layout::create_layout(style_root, 0);
        expect(expected_layout == layout_root);
    });
    // clang-format off

    etest::test("text", [] {
        auto dom_root = dom::create_element_node("html", {}, {
            dom::create_element_node("body", {}, {
                dom::Text{"hello"},
                dom::Text{"goodbye"},
            }),
        });

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {},
            .children = {
                {children[0], {}, {
                    {std::get<dom::Element>(children[0]).children[0], {}, {}},
                    {std::get<dom::Element>(children[0]).children[1], {}, {}},
                }},
            },
        };

        auto expected_layout = layout::LayoutBox{
            .node = &style_root,
            .type = LayoutType::Block,
            .dimensions = {{0, 0, 0, 20}},
            .children = {
                {&style_root.children[0], LayoutType::Block, {{0, 0, 0, 20}}, {
                    {nullptr, LayoutType::AnonymousBlock, {{0, 0, 0, 20}}, {
                        {&style_root.children[0].children[0], LayoutType::Inline, {{0, 0, 0, 10}}, {}},
                        {&style_root.children[0].children[1], LayoutType::Inline, {{0, 10, 0, 10}}, {}},
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

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {{"width", "100px"}},
            .children = {
                {children[0], {}, {
                    {std::get<dom::Element>(children[0]).children[0], {}, {}},
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

    etest::test("min-width", [] {
        auto dom_root = dom::create_element_node("html", {}, {
            dom::create_element_node("body", {}, {
                dom::create_element_node("p", {}, {}),
            }),
        });

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {{"min-width", "100px"}},
            .children = {
                {children[0], {{"min-width", "50px"}}, {
                    {std::get<dom::Element>(children[0]).children[0], {}, {}},
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

        expect(layout::create_layout(style_root, 20) == expected_layout);
    });

    etest::test("max-width", [] {
        auto dom_root = dom::create_element_node("html", {}, {
            dom::create_element_node("body", {}, {
                dom::create_element_node("p", {}, {}),
            }),
        });

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {{"max-width", "100px"}},
            .children = {
                {children[0], {{"max-width", "50px"}}, {
                    {std::get<dom::Element>(children[0]).children[0], {}, {}},
                }},
            },
        };

        auto expected_layout = layout::LayoutBox{
            .node = &style_root,
            .type = LayoutType::Block,
            .dimensions = {{0, 0, 100, 0}},
            .children = {
                {&style_root.children[0], LayoutType::Block, {{0, 0, 50, 0}}, {
                    {&style_root.children[0].children[0], LayoutType::Block, {{0, 0, 50, 0}}, {}},
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

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {{"width", "100px"}},
            .children = {
                {children[0], {{"width", "50px"}}, {
                    {std::get<dom::Element>(children[0]).children[0], {{"width", "25px"}}, {}},
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

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {{"width", "100px"}},
            .children = {
                {children[0], {}, {
                    {std::get<dom::Element>(children[0]).children[0], {}, {}},
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

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {{"height", "100px"}},
            .children = {
                {children[0], {}, {
                    {std::get<dom::Element>(children[0]).children[0], {}, {}},
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

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {},
            .children = {
                {children[0], {}, {
                    {std::get<dom::Element>(children[0]).children[0], {{"height", "25px"}}, {}},
                    {std::get<dom::Element>(children[0]).children[1], {}, {}},
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

    etest::test("min-height is respected", [] {
        auto dom_root = dom::create_element_node("html", {}, {
            dom::create_element_node("body", {}, {
                dom::create_element_node("p", {}, {}),
                dom::create_element_node("p", {}, {}),
            }),
        });

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {{"min-height", "400px"}},
            .children = {
                {children[0], {}, {
                    {std::get<dom::Element>(children[0]).children[0], {{"height", "25px"}}, {}},
                    {std::get<dom::Element>(children[0]).children[1], {}, {}},
                }},
            },
        };

        auto expected_layout = layout::LayoutBox{
            .node = &style_root,
            .type = LayoutType::Block,
            .dimensions = {{0, 0, 0, 400}},
            .children = {
                {&style_root.children[0], LayoutType::Block, {{0, 0, 0, 25}}, {
                    {&style_root.children[0].children[0], LayoutType::Block, {{0, 0, 0, 25}}, {}},
                    {&style_root.children[0].children[1], LayoutType::Block, {{0, 25, 0, 0}}, {}},
                }},
            }
        };

        expect(layout::create_layout(style_root, 0) == expected_layout);
    });

    etest::test("max-height is respected", [] {
        auto dom_root = dom::create_element_node("html", {}, {
            dom::create_element_node("body", {}, {
                dom::create_element_node("p", {}, {}),
                dom::create_element_node("p", {}, {}),
            }),
        });

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {{"max-height", "10px"}},
            .children = {
                {children[0], {}, {
                    {std::get<dom::Element>(children[0]).children[0], {{"height", "400px"}}, {}},
                    {std::get<dom::Element>(children[0]).children[1], {}, {}},
                }},
            },
        };

        auto expected_layout = layout::LayoutBox{
            .node = &style_root,
            .type = LayoutType::Block,
            .dimensions = {{0, 0, 0, 10}},
            .children = {
                {&style_root.children[0], LayoutType::Block, {{0, 0, 0, 400}}, {
                    {&style_root.children[0].children[0], LayoutType::Block, {{0, 0, 0, 400}}, {}},
                    {&style_root.children[0].children[1], LayoutType::Block, {{0, 400, 0, 0}}, {}},
                }},
            }
        };

        expect(layout::create_layout(style_root, 0) == expected_layout);
    });

    etest::test("padding is taken into account", [] {
        auto dom_root = dom::create_element_node("html", {}, {
            dom::create_element_node("body", {}, {
                dom::create_element_node("p", {}, {}),
                dom::create_element_node("p", {}, {}),
            }),
        });

        auto properties = std::vector{
                std::pair{"height"s, "100px"s},
                std::pair{"padding-top"s, "10px"s},
                std::pair{"padding-right"s, "10px"s},
                std::pair{"padding-bottom"s, "10px"s},
                std::pair{"padding-left"s, "10px"s},
        };

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {},
            .children = {
                {children[0], {}, {
                    {std::get<dom::Element>(children[0]).children[0], std::move(properties), {}},
                    {std::get<dom::Element>(children[0]).children[1], {}, {}},
                }},
            },
        };

        auto expected_layout = layout::LayoutBox{
            .node = &style_root,
            .type = LayoutType::Block,
            .dimensions = {{0, 0, 100, 120}},
            .children = {
                {&style_root.children[0], LayoutType::Block, {{0, 0, 100, 120}}, {
                    {&style_root.children[0].children[0], LayoutType::Block, {{10, 10, 80, 100}, {10, 10, 10, 10}, {}, {0, 0, 0, 0}}, {}},
                    {&style_root.children[0].children[1], LayoutType::Block, {{0, 120, 100, 0}}, {}},
                }},
            }
        };

        expect(layout::create_layout(style_root, 100) == expected_layout);
    });

    etest::test("border is taken into account", [] {
        auto dom_root = dom::create_element_node("html", {}, {
            dom::create_element_node("body", {}, {
                dom::create_element_node("p", {}, {}),
                dom::create_element_node("p", {}, {}),
            }),
        });

        auto properties = std::vector{
                std::pair{"height"s, "100px"s},
                std::pair{"border-left-style"s, "solid"s},
                std::pair{"border-right-style"s, "solid"s},
                std::pair{"border-top-style"s, "solid"s},
                std::pair{"border-bottom-style"s, "solid"s},
                std::pair{"border-left-width"s, "10px"s},
                std::pair{"border-right-width"s, "12px"s},
                std::pair{"border-top-width"s, "14px"s},
                std::pair{"border-bottom-width"s, "16px"s},
        };

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {},
            .children = {
                {children[0], {}, {
                    {std::get<dom::Element>(children[0]).children[0], std::move(properties), {}},
                    {std::get<dom::Element>(children[0]).children[1], {}, {}},
                }},
            },
        };

        auto expected_layout = layout::LayoutBox{
            .node = &style_root,
            .type = LayoutType::Block,
            .dimensions = {{0, 0, 100, 130}},
            .children = {
                {&style_root.children[0], LayoutType::Block, {{0, 0, 100, 130}}, {
                    {&style_root.children[0].children[0], LayoutType::Block, {{10, 14, 78, 100}, {}, {10, 12, 14, 16}, {}}, {}},
                    {&style_root.children[0].children[1], LayoutType::Block, {{0, 130, 100, 0}}, {}},
                }},
            }
        };

        expect(layout::create_layout(style_root, 100) == expected_layout);
    });

    etest::test("border is not added if border style is none", [] {
        auto dom_root = dom::create_element_node("html", {}, {
            dom::create_element_node("body", {}, {
                dom::create_element_node("p", {}, {}),
            }),
        });

        auto properties = std::vector{
                std::pair{"height"s, "100px"s},
                std::pair{"border-left-width"s, "10px"s},
                std::pair{"border-right-width"s, "12px"s},
                std::pair{"border-top-width"s, "14px"s},
                std::pair{"border-bottom-width"s, "16px"s},
        };

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {},
            .children = {
                {children[0], {}, {
                    {std::get<dom::Element>(children[0]).children[0], std::move(properties), {}},
                }},
            },
        };

        auto expected_layout = layout::LayoutBox{
            .node = &style_root,
            .type = LayoutType::Block,
            .dimensions = {{0, 0, 100, 100}},
            .children = {
                {&style_root.children[0], LayoutType::Block, {{0, 0, 100, 100}}, {
                    {&style_root.children[0].children[0], LayoutType::Block, {{0, 0, 100, 100}, {}, {}, {}}, {}},
                }},
            }
        };

        expect(layout::create_layout(style_root, 100) == expected_layout);
    });

    etest::test("margin is taken into account", [] {
        auto dom_root = dom::create_element_node("html", {}, {
            dom::create_element_node("body", {}, {
                dom::create_element_node("p", {}, {}),
                dom::create_element_node("p", {}, {}),
            }),
        });

        auto properties = std::vector{
                std::pair{"margin-top"s, "10px"s},
                std::pair{"margin-right"s, "10px"s},
                std::pair{"margin-bottom"s, "10px"s},
                std::pair{"margin-left"s, "10px"s},
        };

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {},
            .children = {
                {children[0], {}, {
                    {std::get<dom::Element>(children[0]).children[0], std::move(properties), {}},
                    {std::get<dom::Element>(children[0]).children[1], {}, {}},
                }},
            },
        };

        auto expected_layout = layout::LayoutBox{
            .node = &style_root,
            .type = LayoutType::Block,
            .dimensions = {{0, 0, 100, 20}},
            .children = {
                {&style_root.children[0], LayoutType::Block, {{0, 0, 100, 20}}, {
                    {&style_root.children[0].children[0], LayoutType::Block, {{10, 10, 80, 0}, {}, {}, {10, 10, 10, 10}}, {}},
                    {&style_root.children[0].children[1], LayoutType::Block, {{0, 20, 100, 0}}, {}},
                }},
            }
        };

        expect(layout::create_layout(style_root, 100) == expected_layout);
    });

    etest::test("auto margin is handled", [] {
        auto dom_root = dom::create_element_node("html", {}, {
            dom::create_element_node("body", {}, {
                dom::create_element_node("p", {}, {}),
            }),
        });

        auto properties = std::vector{
                std::pair{"width"s, "100px"s},
                std::pair{"margin-left"s, "auto"s},
                std::pair{"margin-right"s, "auto"s},
        };

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {},
            .children = {
                {children[0], {}, {
                    {std::get<dom::Element>(children[0]).children[0], std::move(properties), {}},
                }},
            },
        };

        auto expected_layout = layout::LayoutBox{
            .node = &style_root,
            .type = LayoutType::Block,
            .dimensions = {{0, 0, 200, 0}},
            .children = {
                {&style_root.children[0], LayoutType::Block, {{0, 0, 200, 0}}, {
                    {&style_root.children[0].children[0], LayoutType::Block, {{50, 0, 100, 0}, {}, {}, {50, 50, 0, 0}}, {}},
                }},
            }
        };

        expect(layout::create_layout(style_root, 200) == expected_layout);
    });

    etest::test("auto left margin and fixed right margin is handled", [] {
        auto dom_root = dom::create_element_node("html", {}, {
            dom::create_element_node("body", {}, {
                dom::create_element_node("p", {}, {}),
            }),
        });

        auto properties = std::vector{
                std::pair{"width"s, "100px"s},
                std::pair{"margin-left"s, "auto"s},
                std::pair{"margin-right"s, "20px"s},
        };

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {},
            .children = {
                {children[0], {}, {
                    {std::get<dom::Element>(children[0]).children[0], std::move(properties), {}},
                }},
            },
        };

        auto expected_layout = layout::LayoutBox{
            .node = &style_root,
            .type = LayoutType::Block,
            .dimensions = {{0, 0, 200, 0}},
            .children = {
                {&style_root.children[0], LayoutType::Block, {{0, 0, 200, 0}}, {
                    {&style_root.children[0].children[0], LayoutType::Block, {{80, 0, 100, 0}, {}, {}, {80, 20, 0, 0}}, {}},
                }},
            }
        };

        expect(layout::create_layout(style_root, 200) == expected_layout);
    });

    etest::test("fixed left margin and auto right margin is handled", [] {
        auto dom_root = dom::create_element_node("html", {}, {
            dom::create_element_node("body", {}, {
                dom::create_element_node("p", {}, {}),
            }),
        });

        auto properties = std::vector{
                std::pair{"width"s, "100px"s},
                std::pair{"margin-left"s, "75px"s},
                std::pair{"margin-right"s, "auto"s},
        };

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {},
            .children = {
                {children[0], {}, {
                    {std::get<dom::Element>(children[0]).children[0], std::move(properties), {}},
                }},
            },
        };

        auto expected_layout = layout::LayoutBox{
            .node = &style_root,
            .type = LayoutType::Block,
            .dimensions = {{0, 0, 200, 0}},
            .children = {
                {&style_root.children[0], LayoutType::Block, {{0, 0, 200, 0}}, {
                    {&style_root.children[0].children[0], LayoutType::Block, {{75, 0, 100, 0}, {}, {}, {75, 25, 0, 0}}, {}},
                }},
            }
        };

        expect(layout::create_layout(style_root, 200) == expected_layout);
    });

    etest::test("em sizes depend on the font-size", [] {
        auto dom_root = dom::create_element_node("html", {}, {});
        {
            auto style_root = style::StyledNode{
                .node = dom_root,
                .properties = {
                        std::pair{"font-size", "10px"},
                        std::pair{"height", "10em"},
                        std::pair{"width", "10em"},
                },
                .children = {},
            };

            auto expected_layout = layout::LayoutBox{
                .node = &style_root,
                .type = LayoutType::Block,
                .dimensions = {{0, 0, 100, 100}},
                .children = {}
            };

            expect(layout::create_layout(style_root, 1000) == expected_layout);
        }

        // Doubling the font-size should double the width/height.
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {
                    std::pair{"font-size", "20px"},
                    std::pair{"height", "10em"},
                    std::pair{"width", "10em"},
            },
            .children = {},
        };

        auto expected_layout = layout::LayoutBox{
            .node = &style_root,
            .type = LayoutType::Block,
            .dimensions = {{0, 0, 200, 200}},
            .children = {}
        };

        expect(layout::create_layout(style_root, 1000) == expected_layout);
    });

    etest::test("px sizes don't depend on the font-size", [] {
        auto dom_root = dom::create_element_node("html", {}, {});
        {
            auto style_root = style::StyledNode{
                .node = dom_root,
                .properties = {
                        std::pair{"font-size", "10px"},
                        std::pair{"height", "10px"},
                        std::pair{"width", "10px"},
                },
                .children = {},
            };

            auto expected_layout = layout::LayoutBox{
                .node = &style_root,
                .type = LayoutType::Block,
                .dimensions = {{0, 0, 10, 10}},
                .children = {}
            };

            expect(layout::create_layout(style_root, 1000) == expected_layout);
        }

        // Doubling the font-size shouldn't change the width/height.
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {
                    std::pair{"font-size", "20px"},
                    std::pair{"height", "10px"},
                    std::pair{"width", "10px"},
            },
            .children = {},
        };

        auto expected_layout = layout::LayoutBox{
            .node = &style_root,
            .type = LayoutType::Block,
            .dimensions = {{0, 0, 10, 10}},
            .children = {}
        };

        expect(layout::create_layout(style_root, 1000) == expected_layout);
    });

    etest::test("box_at_position", [] {
        auto layout = layout::LayoutBox{
            .node = nullptr,
            .type = LayoutType::Block,
            .dimensions = {{0, 0, 100, 100}},
            .children = {
                {nullptr, LayoutType::Block, {{25, 25, 50, 50}}, {
                    {nullptr, LayoutType::AnonymousBlock, {{30, 30, 5, 5}}, {}},
                    {nullptr, LayoutType::Block, {{45, 45, 5, 5}}, {}},
                }},
            }
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

    etest::test("to_string", [] {
        auto dom_root = dom::create_element_node("html", {}, {
            dom::create_element_node("body", {}, {
                dom::create_element_node("p", {}, {}),
                dom::create_element_node("p", {}, {}),
            }),
        });

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {},
            .children = {
                {children[0], {{"width", "50px"}}, {
                    {std::get<dom::Element>(children[0]).children[0], {{"height", "25px"}}, {}},
                    {std::get<dom::Element>(children[0]).children[1], {{"padding-top", "5px"}, {"padding-right", "15px"}}, {}},
                }},
            },
        };

        auto expected =
                "html\n"
                "block {0,0,0,30} {0,0,0,0} {0,0,0,0}\n"
                "  body\n"
                "  block {0,0,50,30} {0,0,0,0} {0,0,0,0}\n"
                "    p\n"
                "    block {0,0,50,25} {0,0,0,0} {0,0,0,0}\n"
                "    p\n"
                "    block {0,30,35,0} {5,15,0,0} {0,0,0,0}\n";
        expect_eq(to_string(layout::create_layout(style_root, 0)), expected);
    });

    // clang-format on
    etest::test("max-width: none", [] {
        dom::Node dom = dom::Element{.name{"html"}};
        style::StyledNode style{.node{dom}, .properties{{"width", "100px"}, {"max-width", "none"}}};
        layout::LayoutBox expected_layout{.node = &style, .type = LayoutType::Block, .dimensions{{0, 0, 100, 0}}};

        auto layout = layout::create_layout(style, 0);
        expect_eq(layout, expected_layout);
    });

    etest::test("max-height: none", [] {
        dom::Node dom = dom::Element{.name{"html"}};
        style::StyledNode style{.node{dom}, .properties{{"height", "100px"}, {"max-height", "none"}}};
        layout::LayoutBox expected_layout{.node = &style, .type = LayoutType::Block, .dimensions{{0, 0, 0, 100}}};

        auto layout = layout::create_layout(style, 0);
        expect_eq(layout, expected_layout);
    });

    etest::test("height: auto", [] {
        dom::Node dom = dom::Element{.name{"html"}, .children{dom::Element{.name{"p"}}}};
        style::StyledNode style{
                .node{dom},
                .properties{{"height", "auto"}},
                .children{
                        style::StyledNode{
                                .node{std::get<dom::Element>(dom).children[0]},
                                .properties{{"height", "10px"}},
                        },
                },
        };
        layout::LayoutBox expected_layout{
                .node = &style,
                .type = LayoutType::Block,
                .dimensions{{0, 0, 0, 10}},
                .children{layout::LayoutBox{
                        .node = &style.children[0],
                        .type = LayoutType::Block,
                        .dimensions{{0, 0, 0, 10}},
                }},
        };

        auto layout = layout::create_layout(style, 0);
        expect_eq(layout, expected_layout);
    });

    etest::test("get_property", [] {
        dom::Node dom_root = dom::Element{.name{"html"}, .attributes{}, .children{}};
        auto style_root = style::StyledNode{.node = dom_root, .properties = {{"color", "green"}}, .children{}};

        auto layout = layout::create_layout(style_root, 0);
        expect_eq(layout.get_property("color"), "green");
        expect_eq(layout.get_property("background-color"), std::nullopt);
    });

    etest::test("get_property, no backing style node", [] {
        auto layout = layout::LayoutBox{
                .node = nullptr,
        };
        expect_eq(layout.get_property("color"), std::nullopt);
    });

    return etest::run_all_tests();
}
