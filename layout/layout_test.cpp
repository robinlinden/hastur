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
                        {&style_root.children[0].children[0], LayoutType::Inline, {{0, 0, 25, 10}}, {}, "hello"sv},
                        {&style_root.children[0].children[1], LayoutType::Inline, {{25, 0, 35, 10}}, {}, "goodbye"sv},
                    }},
                }},
            }
        };

        auto layout_root = layout::create_layout(style_root, 0);
        expect(expected_layout == layout_root);

        expect_eq(expected_layout.children.at(0).children.at(0).children.at(0).text(), "hello");
        expect_eq(expected_layout.children.at(0).children.at(0).children.at(1).text(), "goodbye");
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
        // https://drafts.csswg.org/css-fonts-4/#absolute-size-mapping
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

        auto get_text_dimensions = [](layout::LayoutBox const &layout) {
            require_eq(layout.children.size(), std::size_t{1});
            require_eq(layout.children[0].children.size(), std::size_t{1});
            auto const &content_dims = layout.children[0].children[0].dimensions.content;
            return std::pair{content_dims.width, content_dims.height};
        };

        auto single_line_layout = layout::create_layout(style, 1000).value();
        auto single_line_layout_dims = get_text_dimensions(single_line_layout);
        require(single_line_layout_dims.second > 0);

        std::get<dom::Text>(std::get<dom::Element>(dom).children[0]).text = "hi\nhi"s;
        auto two_line_layout = layout::create_layout(style, 1000).value();
        auto two_line_layout_dims = get_text_dimensions(two_line_layout);

        expect(two_line_layout_dims.second >= 2 * single_line_layout_dims.second);
        expect_eq(single_line_layout_dims.first, two_line_layout_dims.first);
    });

    etest::test("display:none on root node", [] {
        dom::Node dom = dom::Element{.name{"html"}};
        style::StyledNode style{
                .node{dom},
                .properties{{css::PropertyId::Display, "none"}},
        };

        expect_eq(layout::create_layout(style, 0), std::nullopt);
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

    etest::test("whitespace collapsing: simple", [] {
        constexpr auto kText = "   hello   "sv;
        constexpr auto kCollapsedText = util::trim(kText);
        constexpr auto kTextWidth = kCollapsedText.length() * 5;

        dom::Element p{.name{"p"}, .children{dom::Text{std::string{kText}}}};
        dom::Node html = dom::Element{.name{"html"}, .children{std::move(p)}};
        dom::Element const &html_element = std::get<dom::Element>(html);

        style::StyledNode p_style{
                .node{html_element.children[0]},
                .properties{{css::PropertyId::Display, "inline"}},
                .children{style::StyledNode{std::get<dom::Element>(html_element.children.at(0)).children.at(0)}},
        };
        style::StyledNode style{
                .node{html},
                .properties{{css::PropertyId::Display, "block"}, {css::PropertyId::FontSize, "10px"}},
                .children{std::move(p_style)},
        };
        set_up_parent_ptrs(style);

        layout::LayoutBox p_layout{
                .node = &style.children.at(0),
                .type = LayoutType::Inline,
                .dimensions{{0, 0, kTextWidth, 10}},
                .children{layout::LayoutBox{
                        .node = &style.children.at(0).children.at(0),
                        .type = LayoutType::Inline,
                        .dimensions{{0, 0, kTextWidth, 10}},
                        .layout_text{kCollapsedText},
                }},
        };
        layout::LayoutBox expected_layout{
                .node = &style,
                .type = LayoutType::Block,
                .dimensions{{0, 0, 1234, 10}},
                .children{layout::LayoutBox{
                        .node = nullptr,
                        .type = LayoutType::AnonymousBlock,
                        .dimensions{{0, 0, kTextWidth, 10}},
                        .children{std::move(p_layout)},
                }},
        };

        auto actual = layout::create_layout(style, 1234);
        expect_eq(actual, expected_layout);
    });

    etest::test("whitespace collapsing: text split across multiple inline elements", [] {
        constexpr auto kFirstText = "   cr "sv;
        constexpr auto kSecondText = " lf   "sv;
        constexpr auto kCollapsedFirst = util::trim_start(kFirstText);
        constexpr auto kFirstWidth = kCollapsedFirst.length() * 5;
        constexpr auto kCollapsedSecond = util::trim(kSecondText);
        constexpr auto kSecondWidth = kCollapsedSecond.length() * 5;

        dom::Element a{.name{"a"}, .children{dom::Text{std::string{kSecondText}}}};
        dom::Element p{.name{"p"}, .children{dom::Text{std::string{kFirstText}}, std::move(a)}};
        dom::Node html = dom::Element{.name{"html"}, .children{std::move(p)}};
        auto const &html_element = std::get<dom::Element>(html);
        auto const &p_element = std::get<dom::Element>(html_element.children.at(0));
        auto const &a_element = std::get<dom::Element>(p_element.children.at(1));

        style::StyledNode a_style{
                .node{a_element},
                .properties{{css::PropertyId::Display, "inline"}},
                .children{style::StyledNode{a_element.children.at(0)}},
        };
        style::StyledNode p_style{
                .node{p_element},
                .properties{{css::PropertyId::Display, "inline"}},
                .children{style::StyledNode{p_element.children.at(0)}, std::move(a_style)},
        };
        style::StyledNode style{
                .node{html},
                .properties{{css::PropertyId::Display, "block"}, {css::PropertyId::FontSize, "10px"}},
                .children{std::move(p_style)},
        };
        set_up_parent_ptrs(style);

        layout::LayoutBox a_layout{
                .node = &style.children.at(0).children.at(1),
                .type = LayoutType::Inline,
                .dimensions{{kFirstWidth, 0, kSecondWidth, 10}},
                .children{layout::LayoutBox{
                        .node = &style.children.at(0).children.at(1).children.at(0),
                        .type = LayoutType::Inline,
                        .dimensions{{kFirstWidth, 0, kSecondWidth, 10}},
                        .layout_text{kCollapsedSecond},
                }},
        };
        layout::LayoutBox p_layout{
                .node = &style.children.at(0),
                .type = LayoutType::Inline,
                .dimensions{{0, 0, kFirstWidth + kSecondWidth, 10}},
                .children{
                        layout::LayoutBox{
                                .node = &style.children.at(0).children.at(0),
                                .type = LayoutType::Inline,
                                .dimensions{{0, 0, kFirstWidth, 10}},
                                .layout_text{kCollapsedFirst},
                        },
                        std::move(a_layout),
                },
        };
        layout::LayoutBox expected_layout{
                .node = &style,
                .type = LayoutType::Block,
                .dimensions{{0, 0, 1234, 10}},
                .children{layout::LayoutBox{
                        .node = nullptr,
                        .type = LayoutType::AnonymousBlock,
                        .dimensions{{0, 0, kFirstWidth + kSecondWidth, 10}},
                        .children{std::move(p_layout)},
                }},
        };

        auto actual = layout::create_layout(style, 1234);
        expect_eq(actual, expected_layout);
    });

    etest::test("whitespace collapsing: text separated by a block element", [] {
        constexpr auto kFirstText = "  a  "sv;
        constexpr auto kSecondText = "  b  "sv;
        constexpr auto kCollapsedFirst = util::trim(kFirstText);
        constexpr auto kFirstWidth = kCollapsedFirst.length() * 5;
        constexpr auto kCollapsedSecond = util::trim(kSecondText);
        constexpr auto kSecondWidth = kCollapsedSecond.length() * 5;

        dom::Element first{.name{"p"}, .children{dom::Text{std::string{kFirstText}}}};
        dom::Element block{.name{"div"}};
        dom::Element second{.name{"p"}, .children{dom::Text{std::string{kSecondText}}}};
        dom::Node html = dom::Element{.name{"html"}, .children{std::move(first), std::move(block), std::move(second)}};
        auto const &html_element = std::get<dom::Element>(html);
        auto const &first_text_element = std::get<dom::Element>(html_element.children.at(0));
        auto const &block_element = std::get<dom::Element>(html_element.children.at(1));
        auto const &second_text_element = std::get<dom::Element>(html_element.children.at(2));

        style::StyledNode first_style{
                .node{first_text_element},
                .properties{{css::PropertyId::Display, "inline"}},
                .children{style::StyledNode{first_text_element.children.at(0)}},
        };
        style::StyledNode block_style{.node{block_element}, .properties{{css::PropertyId::Display, "block"}}};
        style::StyledNode second_style{
                .node{second_text_element},
                .properties{{css::PropertyId::Display, "inline"}},
                .children{style::StyledNode{second_text_element.children.at(0)}},
        };
        style::StyledNode style{
                .node{html},
                .properties{{css::PropertyId::Display, "block"}, {css::PropertyId::FontSize, "10px"}},
                .children{std::move(first_style), std::move(block_style), std::move(second_style)},
        };
        set_up_parent_ptrs(style);

        layout::LayoutBox first_layout{
                .node = &style.children.at(0),
                .type = LayoutType::Inline,
                .dimensions{{0, 0, kFirstWidth, 10}},
                .children{layout::LayoutBox{
                        .node = &style.children.at(0).children.at(0),
                        .type = LayoutType::Inline,
                        .dimensions{{0, 0, kFirstWidth, 10}},
                        .layout_text{kCollapsedFirst},
                }},
        };
        layout::LayoutBox second_layout{
                .node = &style.children.at(2),
                .type = LayoutType::Inline,
                .dimensions{{0, 10, kSecondWidth, 10}},
                .children{layout::LayoutBox{
                        .node = &style.children.at(2).children.at(0),
                        .type = LayoutType::Inline,
                        .dimensions{{0, 10, kSecondWidth, 10}},
                        .layout_text{kCollapsedSecond},
                }},
        };
        layout::LayoutBox expected_layout{
                .node = &style,
                .type = LayoutType::Block,
                .dimensions{{0, 0, 1234, 20}},
                .children{
                        layout::LayoutBox{
                                .node = nullptr,
                                .type = LayoutType::AnonymousBlock,
                                .dimensions{{0, 0, kFirstWidth, 10}},
                                .children{std::move(first_layout)},
                        },
                        layout::LayoutBox{
                                .node = &style.children.at(1),
                                .type = LayoutType::Block,
                                .dimensions{{0, 10, 1234, 0}},
                        },
                        layout::LayoutBox{
                                .node = nullptr,
                                .type = LayoutType::AnonymousBlock,
                                .dimensions{{0, 10, kSecondWidth, 10}},
                                .children{std::move(second_layout)},
                        },
                },
        };

        auto actual = layout::create_layout(style, 1234);
        expect_eq(actual, expected_layout);
    });

    return etest::run_all_tests();
}
