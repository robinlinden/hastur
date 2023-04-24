// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "layout/layout.h"

#include "etest/etest.h"

#include <string_view>
#include <utility>

using namespace std::literals;
using etest::expect;
using etest::expect_eq;
using etest::require;
using etest::require_eq;
using layout::LayoutType;

namespace {

// Until we have a nicer tree-creation abstraction for the tests, this needs to
// be called if a test relies on property inheritance.
void set_up_parent_ptrs(style::StyledNode &parent) {
    for (auto &child : parent.children) {
        child.parent = &parent;
        set_up_parent_ptrs(child);
    }
}

// TODO(robinlinden): Remove.
dom::Node create_element_node(std::string_view name, dom::AttrMap attrs, std::vector<dom::Node> children) {
    return dom::Element{std::string{name}, std::move(attrs), std::move(children)};
}

} // namespace

// TODO(robinlinden): clang-format doesn't get along well with how I structured
// the trees in these test cases.

// clang-format off

int main() {
    etest::test("simple tree", [] {
        auto dom_root = create_element_node("html", {}, {
            create_element_node("head", {}, {}),
            create_element_node("body", {}, {
                create_element_node("p", {}, {}),
            }),
        });

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {{css::PropertyId::Display, "block"}},
            .children = {
                {children[0], {{css::PropertyId::Display, "block"}}, {}},
                {children[1], {{css::PropertyId::Display, "block"}}, {
                    {std::get<dom::Element>(children[1]).children[0], {{css::PropertyId::Display, "block"}}, {}},
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
        auto dom_root = create_element_node("html", {}, {
            create_element_node("head", {}, {}),
            create_element_node("body", {}, {
                create_element_node("p", {}, {}),
            }),
        });

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {{css::PropertyId::Display, "block"}},
            .children = {
                {children[0], {{css::PropertyId::Display, "none"}}, {}},
                {children[1], {{css::PropertyId::Display, "block"}}, {

                    {std::get<dom::Element>(children[1]).children[0], {{css::PropertyId::Display, "block"}}, {}},
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
        auto dom_root = create_element_node("html", {}, {
            create_element_node("head", {}, {}),
            create_element_node("body", {}, {
                create_element_node("p", {}, {}),
            }),
        });

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {{css::PropertyId::Display, "block"}},
            .children = {
                {children[0], {{css::PropertyId::Display, "inline"}}, {}},
                {children[1], {{css::PropertyId::Display, "inline"}}, {
                    {std::get<dom::Element>(children[1]).children[0], {}, {}},
                }},
            },
        };

        auto expected_layout = layout::LayoutBox{
            .node = &style_root,
            .type = LayoutType::Block,
            .dimensions = {},
            .children = {
                {nullptr, LayoutType::AnonymousBlock, {}, {
                    {&style_root.children[0], LayoutType::Inline, {}, {}},
                    {&style_root.children[1], LayoutType::Inline, {}, {
                        {&style_root.children[1].children[0], LayoutType::Inline, {}, {}},
                    }},
                }},
            }
        };

        auto layout_root = layout::create_layout(style_root, 0);
        expect(expected_layout == layout_root);
    });

    // clang-format on
    etest::test("inline in inline don't get wrapped in anon-blocks", [] {
        auto dom_root = create_element_node("span", {}, {create_element_node("span", {}, {})});

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
                .node = dom_root,
                .properties = {{css::PropertyId::Display, "inline"}},
                .children = {{children[0], {{css::PropertyId::Display, "inline"}}, {}}},
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
        auto dom_root = create_element_node("html", {}, {
            create_element_node("body", {}, {
                dom::Text{"hello"},
                dom::Text{"goodbye"},
            }),
        });

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {{css::PropertyId::Display, "block"}, {css::PropertyId::FontSize, "10px"}},
            .children = {
                {children[0], {{css::PropertyId::Display, "block"}}, {
                    {std::get<dom::Element>(children[0]).children[0], {}, {}},
                    {std::get<dom::Element>(children[0]).children[1], {}, {}},
                }},
            },
        };
        set_up_parent_ptrs(style_root);

        auto expected_layout = layout::LayoutBox{
            .node = &style_root,
            .type = LayoutType::Block,
            .dimensions = {{0, 0, 0, 10}},
            .children = {
                {&style_root.children[0], LayoutType::Block, {{0, 0, 0, 10}}, {
                    {nullptr, LayoutType::AnonymousBlock, {{0, 0, 60, 10}}, {
                        {&style_root.children[0].children[0], LayoutType::Inline, {{0, 0, 25, 10}}, {}},
                        {&style_root.children[0].children[1], LayoutType::Inline, {{25, 0, 35, 10}}, {}},
                    }},
                }},
            }
        };

        auto layout_root = layout::create_layout(style_root, 0);
        expect(expected_layout == layout_root);
    });

    etest::test("simple width", [] {
        auto dom_root = create_element_node("html", {}, {
            create_element_node("body", {}, {
                create_element_node("p", {}, {}),
            }),
        });

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {{css::PropertyId::Width, "100px"}, {css::PropertyId::Display, "block"}},
            .children = {
                {children[0], {{css::PropertyId::Display, "block"}}, {
                    {std::get<dom::Element>(children[0]).children[0], {{css::PropertyId::Display, "block"}}, {}},
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
        auto dom_root = create_element_node("html", {}, {
            create_element_node("body", {}, {
                create_element_node("p", {}, {}),
            }),
        });

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {{css::PropertyId::MinWidth, "100px"}, {css::PropertyId::Display, "block"}},
            .children = {
                {children[0], {{css::PropertyId::MinWidth, "50px"}, {css::PropertyId::Display, "block"}}, {
                    {std::get<dom::Element>(children[0]).children[0], {{css::PropertyId::Display, "block"}}, {}},
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
        auto dom_root = create_element_node("html", {}, {
            create_element_node("body", {}, {
                create_element_node("p", {}, {}),
            }),
        });

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {{css::PropertyId::MaxWidth, "100px"}, {css::PropertyId::Display, "block"}},
            .children = {
                {children[0], {{css::PropertyId::MaxWidth, "50px"}, {css::PropertyId::Display, "block"}}, {
                    {std::get<dom::Element>(children[0]).children[0], {{css::PropertyId::Display, "block"}}, {}},
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
        auto dom_root = create_element_node("html", {}, {
            create_element_node("body", {}, {
                create_element_node("p", {}, {}),
            }),
        });

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {{css::PropertyId::Width, "100px"}, {css::PropertyId::Display, "block"}},
            .children = {
                {children[0], {{css::PropertyId::Width, "50px"}, {css::PropertyId::Display, "block"}}, {
                    {std::get<dom::Element>(children[0]).children[0], {{css::PropertyId::Width, "25px"}, {css::PropertyId::Display, "block"}}, {}},
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
        auto dom_root = create_element_node("html", {}, {
            create_element_node("body", {}, {
                create_element_node("p", {}, {}),
            }),
        });

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {{css::PropertyId::Width, "100px"}, {css::PropertyId::Display, "block"}},
            .children = {
                {children[0], {{css::PropertyId::Display, "block"}}, {
                    {std::get<dom::Element>(children[0]).children[0], {{css::PropertyId::Display, "block"}}, {}},
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
        auto dom_root = create_element_node("html", {}, {
            create_element_node("body", {}, {
                create_element_node("p", {}, {}),
            }),
        });

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {{css::PropertyId::Height, "100px"}, {css::PropertyId::Display, "block"}},
            .children = {
                {children[0], {{css::PropertyId::Display, "block"}}, {
                    {std::get<dom::Element>(children[0]).children[0], {{css::PropertyId::Display, "block"}}, {}},
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
        auto dom_root = create_element_node("html", {}, {
            create_element_node("body", {}, {
                create_element_node("p", {}, {}),
                create_element_node("p", {}, {}),
            }),
        });

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {{css::PropertyId::Display, "block"}},
            .children = {
                {children[0], {{css::PropertyId::Display, "block"}}, {
                    {std::get<dom::Element>(children[0]).children[0], {{css::PropertyId::Height, "25px"}, {css::PropertyId::Display, "block"}}, {}},
                    {std::get<dom::Element>(children[0]).children[1], {{css::PropertyId::Display, "block"}}, {}},
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
        auto dom_root = create_element_node("html", {}, {
            create_element_node("body", {}, {
                create_element_node("p", {}, {}),
                create_element_node("p", {}, {}),
            }),
        });

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {{css::PropertyId::MinHeight, "400px"}, {css::PropertyId::Display, "block"}},
            .children = {
                {children[0], {{css::PropertyId::Display, "block"}}, {
                    {std::get<dom::Element>(children[0]).children[0], {{css::PropertyId::Height, "25px"}, {css::PropertyId::Display, "block"}}, {}},
                    {std::get<dom::Element>(children[0]).children[1], {{css::PropertyId::Display, "block"}}, {}},
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
        auto dom_root = create_element_node("html", {}, {
            create_element_node("body", {}, {
                create_element_node("p", {}, {}),
                create_element_node("p", {}, {}),
            }),
        });

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {{css::PropertyId::MaxHeight, "10px"}, {css::PropertyId::Display, "block"}},
            .children = {
                {children[0], {{css::PropertyId::Display, "block"}}, {
                    {std::get<dom::Element>(children[0]).children[0], {{css::PropertyId::Height, "400px"}, {css::PropertyId::Display, "block"}}, {}},
                    {std::get<dom::Element>(children[0]).children[1], {{css::PropertyId::Display, "block"}}, {}},
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
        auto dom_root = create_element_node("html", {}, {
            create_element_node("body", {}, {
                create_element_node("p", {}, {}),
                create_element_node("p", {}, {}),
            }),
        });

        auto properties = std::vector{
                std::pair{css::PropertyId::Display, "block"s},
                std::pair{css::PropertyId::Height, "100px"s},
                std::pair{css::PropertyId::PaddingTop, "10px"s},
                std::pair{css::PropertyId::PaddingRight, "10px"s},
                std::pair{css::PropertyId::PaddingBottom, "10px"s},
                std::pair{css::PropertyId::PaddingLeft, "10px"s},
        };

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {{css::PropertyId::Display, "block"}},
            .children = {
                {children[0], {{css::PropertyId::Display, "block"}}, {
                    {std::get<dom::Element>(children[0]).children[0], std::move(properties), {}},
                    {std::get<dom::Element>(children[0]).children[1], {{css::PropertyId::Display, "block"}}, {}},
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
        auto dom_root = create_element_node("html", {}, {
            create_element_node("body", {}, {
                create_element_node("p", {}, {}),
                create_element_node("p", {}, {}),
            }),
        });

        auto properties = std::vector{
                std::pair{css::PropertyId::Display, "block"s},
                std::pair{css::PropertyId::Height, "100px"s},
                std::pair{css::PropertyId::BorderLeftStyle, "solid"s},
                std::pair{css::PropertyId::BorderRightStyle, "solid"s},
                std::pair{css::PropertyId::BorderTopStyle, "solid"s},
                std::pair{css::PropertyId::BorderBottomStyle, "solid"s},
                std::pair{css::PropertyId::BorderLeftWidth, "10px"s},
                std::pair{css::PropertyId::BorderRightWidth, "12px"s},
                std::pair{css::PropertyId::BorderTopWidth, "14px"s},
                std::pair{css::PropertyId::BorderBottomWidth, "16px"s},
        };

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {{css::PropertyId::Display, "block"}},
            .children = {
                {children[0], {{css::PropertyId::Display, "block"}}, {
                    {std::get<dom::Element>(children[0]).children[0], std::move(properties), {}},
                    {std::get<dom::Element>(children[0]).children[1], {{css::PropertyId::Display, "block"}}, {}},
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
        auto dom_root = create_element_node("html", {}, {
            create_element_node("body", {}, {
                create_element_node("p", {}, {}),
            }),
        });

        auto properties = std::vector{
                std::pair{css::PropertyId::Display, "block"s},
                std::pair{css::PropertyId::Height, "100px"s},
                std::pair{css::PropertyId::BorderLeftWidth, "10px"s},
                std::pair{css::PropertyId::BorderRightWidth, "12px"s},
                std::pair{css::PropertyId::BorderTopWidth, "14px"s},
                std::pair{css::PropertyId::BorderBottomWidth, "16px"s},
        };

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {{css::PropertyId::Display, "block"}},
            .children = {
                {children[0], {{css::PropertyId::Display, "block"}}, {
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
        auto dom_root = create_element_node("html", {}, {
            create_element_node("body", {}, {
                create_element_node("p", {}, {}),
                create_element_node("p", {}, {}),
            }),
        });

        auto properties = std::vector{
                std::pair{css::PropertyId::Display, "block"s},
                std::pair{css::PropertyId::MarginTop, "10px"s},
                std::pair{css::PropertyId::MarginRight, "10px"s},
                std::pair{css::PropertyId::MarginBottom, "10px"s},
                std::pair{css::PropertyId::MarginLeft, "10px"s},
        };

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {{css::PropertyId::Display, "block"}},
            .children = {
                {children[0], {{css::PropertyId::Display, "block"}}, {
                    {std::get<dom::Element>(children[0]).children[0], std::move(properties), {}},
                    {std::get<dom::Element>(children[0]).children[1], {{css::PropertyId::Display, "block"}}, {}},
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
        auto dom_root = create_element_node("html", {}, {
            create_element_node("body", {}, {
                create_element_node("p", {}, {}),
            }),
        });

        auto properties = std::vector{
                std::pair{css::PropertyId::Display, "block"s},
                std::pair{css::PropertyId::Width, "100px"s},
                std::pair{css::PropertyId::MarginLeft, "auto"s},
                std::pair{css::PropertyId::MarginRight, "auto"s},
        };

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {{css::PropertyId::Display, "block"}},
            .children = {
                {children[0], {{css::PropertyId::Display, "block"}}, {
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
        auto dom_root = create_element_node("html", {}, {
            create_element_node("body", {}, {
                create_element_node("p", {}, {}),
            }),
        });

        auto properties = std::vector{
                std::pair{css::PropertyId::Display, "block"s},
                std::pair{css::PropertyId::Width, "100px"s},
                std::pair{css::PropertyId::MarginLeft, "auto"s},
                std::pair{css::PropertyId::MarginRight, "20px"s},
        };

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {{css::PropertyId::Display, "block"}},
            .children = {
                {children[0], {{css::PropertyId::Display, "block"}}, {
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
        auto dom_root = create_element_node("html", {}, {
            create_element_node("body", {}, {
                create_element_node("p", {}, {}),
            }),
        });

        auto properties = std::vector{
                std::pair{css::PropertyId::Display, "block"s},
                std::pair{css::PropertyId::Width, "100px"s},
                std::pair{css::PropertyId::MarginLeft, "75px"s},
                std::pair{css::PropertyId::MarginRight, "auto"s},
        };

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {{css::PropertyId::Display, "block"}},
            .children = {
                {children[0], {{css::PropertyId::Display, "block"}}, {
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
        auto dom_root = create_element_node("html", {}, {});
        {
            auto style_root = style::StyledNode{
                .node = dom_root,
                .properties = {
                        std::pair{css::PropertyId::Display, "block"},
                        std::pair{css::PropertyId::FontSize, "10px"},
                        std::pair{css::PropertyId::Height, "10em"},
                        std::pair{css::PropertyId::Width, "10em"},
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
                    std::pair{css::PropertyId::Display, "block"},
                    std::pair{css::PropertyId::FontSize, "20px"},
                    std::pair{css::PropertyId::Height, "10em"},
                    std::pair{css::PropertyId::Width, "10em"},
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
        auto dom_root = create_element_node("html", {}, {});
        {
            auto style_root = style::StyledNode{
                .node = dom_root,
                .properties = {
                        std::pair{css::PropertyId::Display, "block"},
                        std::pair{css::PropertyId::FontSize, "10px"},
                        std::pair{css::PropertyId::Height, "10px"},
                        std::pair{css::PropertyId::Width, "10px"},
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
                    std::pair{css::PropertyId::Display, "block"},
                    std::pair{css::PropertyId::FontSize, "20px"},
                    std::pair{css::PropertyId::Height, "10px"},
                    std::pair{css::PropertyId::Width, "10px"},
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
        auto dom_root = create_element_node("html", {}, {
            create_element_node("body", {}, {
                create_element_node("p", {}, {}),
                create_element_node("p", {}, {}),
            }),
        });

        auto const &children = std::get<dom::Element>(dom_root).children;
        auto style_root = style::StyledNode{
            .node = dom_root,
            .properties = {{css::PropertyId::Display, "block"}},
            .children = {
                {children[0], {{css::PropertyId::Width, "50px"}, {css::PropertyId::Display, "block"}}, {
                    {std::get<dom::Element>(children[0]).children[0], {{css::PropertyId::Height, "25px"}, {css::PropertyId::Display, "block"}}, {}},
                    {std::get<dom::Element>(children[0]).children[1], {{css::PropertyId::PaddingTop, "5px"}, {css::PropertyId::PaddingRight, "15px"}, {css::PropertyId::Display, "block"}}, {}},
                }},
            },
        };

        auto const *expected =
                "html\n"
                "block {0,0,0,30} {0,0,0,0} {0,0,0,0}\n"
                "  body\n"
                "  block {0,0,50,30} {0,0,0,0} {0,0,0,0}\n"
                "    p\n"
                "    block {0,0,50,25} {0,0,0,0} {0,0,0,0}\n"
                "    p\n"
                "    block {0,30,35,0} {5,15,0,0} {0,0,0,0}\n";
        expect_eq(to_string(layout::create_layout(style_root, 0).value()), expected);
    });

    // clang-format on
    etest::test("max-width: none", [] {
        dom::Node dom = dom::Element{.name{"html"}};
        style::StyledNode style{
                .node{dom},
                .properties{{css::PropertyId::Display, "block"},
                        {css::PropertyId::Width, "100px"},
                        {css::PropertyId::MaxWidth, "none"}},
        };
        layout::LayoutBox expected_layout{.node = &style, .type = LayoutType::Block, .dimensions{{0, 0, 100, 0}}};

        auto layout = layout::create_layout(style, 0);
        expect_eq(layout, expected_layout);
    });

    etest::test("max-height: none", [] {
        dom::Node dom = dom::Element{.name{"html"}};
        style::StyledNode style{
                .node{dom},
                .properties{{css::PropertyId::Display, "block"},
                        {css::PropertyId::Height, "100px"},
                        {css::PropertyId::MaxHeight, "none"}},
        };
        layout::LayoutBox expected_layout{.node = &style, .type = LayoutType::Block, .dimensions{{0, 0, 0, 100}}};

        auto layout = layout::create_layout(style, 0);
        expect_eq(layout, expected_layout);
    });

    etest::test("height: auto", [] {
        dom::Node dom = dom::Element{.name{"html"}, .children{dom::Element{.name{"p"}}}};
        style::StyledNode style{
                .node{dom},
                .properties{{css::PropertyId::Display, "block"}, {css::PropertyId::Height, "auto"}},
                .children{
                        style::StyledNode{
                                .node{std::get<dom::Element>(dom).children[0]},
                                .properties{{css::PropertyId::Display, "block"}, {css::PropertyId::Height, "10px"}},
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

    etest::test("font-size absolute value keywords", [] {
        dom::Node dom = dom::Element{.name{"html"}, .children{dom::Text{"hi"}}};
        style::StyledNode style{
                .node{dom},
                .properties{{css::PropertyId::Display, "block"}, {css::PropertyId::FontSize, "medium"}},
                .children{
                        style::StyledNode{.node{std::get<dom::Element>(dom).children[0]}},
                },
        };
        set_up_parent_ptrs(style);

        auto medium_layout = layout::create_layout(style, 1000).value();
        style.properties = {{css::PropertyId::Display, "block"}, {css::PropertyId::FontSize, "xxx-large"}};
        auto xxxlarge_layout = layout::create_layout(style, 1000).value();

        auto get_text_width = [](layout::LayoutBox const &layout) {
            require_eq(layout.children.size(), std::size_t{1});
            require_eq(layout.children[0].children.size(), std::size_t{1});
            return layout.children[0].children[0].dimensions.content.width;
        };

        auto medium_layout_width = get_text_width(medium_layout);
        auto xxxlarge_layout_width = get_text_width(xxxlarge_layout);
        expect(medium_layout_width > 0);

        // font-size: xxx-large should be 3x font-size: medium.
        // https://w3c.github.io/csswg-drafts/css-fonts-4/#absolute-size-mapping
        expect_eq(medium_layout_width * 3, xxxlarge_layout_width);
    });

    etest::test("invalid size", [] {
        dom::Node dom = dom::Element{.name{"html"}};
        style::StyledNode style{
                .node{dom},
                .properties{{css::PropertyId::Display, "block"}, {css::PropertyId::Height, "no"}},
        };

        layout::LayoutBox expected_layout{
                .node = &style,
                .type = LayoutType::Block,
                .dimensions{{0, 0, 0, 0}},
        };

        auto layout = layout::create_layout(style, 0);
        expect_eq(layout, expected_layout);
    });

    etest::test("unhandled unit", [] {
        dom::Node dom = dom::Element{.name{"html"}};
        style::StyledNode style{
                .node{dom},
                .properties{{css::PropertyId::Display, "block"}, {css::PropertyId::Height, "0notarealunit"}},
        };

        layout::LayoutBox expected_layout{
                .node = &style,
                .type = LayoutType::Block,
                .dimensions{{0, 0, 0, 0}},
        };

        auto layout = layout::create_layout(style, 0);
        expect_eq(layout, expected_layout);
    });

    etest::test("get_property", [] {
        dom::Node dom_root = dom::Element{.name{"html"}, .attributes{}, .children{}};
        auto style_root =
                style::StyledNode{.node = dom_root, .properties = {{css::PropertyId::Color, "green"}}, .children{}};

        auto layout = layout::create_layout(style_root, 0).value();
        expect_eq(layout.get_property<css::PropertyId::Color>(), gfx::Color::from_css_name("green"));
        expect_eq(layout.get_property<css::PropertyId::BackgroundColor>(), gfx::Color::from_css_name("transparent"));
    });

    etest::test("border-width keywords", [] {
        dom::Node dom = dom::Element{.name{"html"}};
        style::StyledNode style{
                .node{dom},
                .properties{{css::PropertyId::Display, "block"},
                        {css::PropertyId::BorderLeftStyle, "solid"},
                        {css::PropertyId::BorderLeftWidth, "thin"}},
        };

        auto layout = layout::create_layout(style, 0).value();
        expect_eq(layout.dimensions.border, geom::EdgeSize{.left = 3});
    });

    etest::test("text with newlines in", [] {
        dom::Node dom = dom::Element{.name{"html"}, .children{dom::Text{"hi"}}};
        style::StyledNode style{
                .node{dom},
                .properties{{css::PropertyId::Display, "block"}},
                .children{style::StyledNode{.node{std::get<dom::Element>(dom).children[0]}}},
        };
        set_up_parent_ptrs(style);

        auto get_text_height = [](layout::LayoutBox const &layout) {
            require_eq(layout.children.size(), std::size_t{1});
            require_eq(layout.children[0].children.size(), std::size_t{1});
            return layout.children[0].children[0].dimensions.content.height;
        };

        auto single_line_layout = layout::create_layout(style, 1000).value();
        auto single_line_layout_height = get_text_height(single_line_layout);
        require(single_line_layout_height > 0);

        std::get<dom::Text>(std::get<dom::Element>(dom).children[0]).text = "hi\nbye"s;
        auto two_line_layout = layout::create_layout(style, 1000).value();
        auto two_line_layout_height = get_text_height(two_line_layout);

        expect(two_line_layout_height >= 2 * single_line_layout_height);
    });

    etest::test("display:none on root node", [] {
        dom::Node dom = dom::Element{.name{"html"}};
        style::StyledNode style{
                .node{dom},
                .properties{{css::PropertyId::Display, "none"}},
        };

        expect_eq(layout::create_layout(style, 0), std::nullopt);
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

    etest::test("rem units", [] {
        dom::Node dom = dom::Element{"html", {}, {dom::Element{"div"}}};
        auto const &div = std::get<dom::Element>(dom).children[0];
        style::StyledNode style{
                .node{dom},
                .properties{{css::PropertyId::FontSize, "10px"}, {css::PropertyId::Display, "block"}},
                .children{
                        style::StyledNode{
                                .node{div},
                                .properties{{css::PropertyId::Width, "2rem"}, {css::PropertyId::Display, "block"}},
                        },
                },
        };
        set_up_parent_ptrs(style);

        auto layout = layout::create_layout(style, 1000).value();
        expect_eq(layout.children.at(0).dimensions.border_box().width, 20);

        style.properties.at(0).second = "16px";
        layout = layout::create_layout(style, 1000).value();
        expect_eq(layout.children.at(0).dimensions.border_box().width, 32);
    });

    return etest::run_all_tests();
}
