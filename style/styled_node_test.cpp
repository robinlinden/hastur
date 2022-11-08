// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "style/styled_node.h"

#include "etest/etest.h"

using namespace std::literals;
using etest::expect;
using etest::expect_eq;
using etest::require;

int main() {
    etest::test("get_property", [] {
        dom::Node dom_node = dom::Element{"dummy"s};
        style::StyledNode styled_node{
                .node = dom_node,
                .properties = {{css::PropertyId::Width, "15px"s}},
                .children = {},
        };

        expect(styled_node.get_property(css::PropertyId::Width) == "15px"sv);
    });

    etest::test("property inheritance", [] {
        dom::Node dom_node = dom::Element{"dummy"s};
        style::StyledNode root{
                .node = dom_node,
                .properties = {{css::PropertyId::FontSize, "15em"s}, {css::PropertyId::Width, "0px"s}},
                .children = {},
        };

        auto &child = root.children.emplace_back(style::StyledNode{dom_node, {}, {}, &root});

        // Not inherited, returns the initial value.
        expect_eq(child.get_property(css::PropertyId::Width), "auto"sv);

        // Inherited, returns the parent's value.
        expect_eq(child.get_property(css::PropertyId::FontSize), "15em"sv);
    });

    etest::test("initial css keyword", [] {
        dom::Node dom_node = dom::Element{"dummy"s};
        style::StyledNode root{
                .node = dom_node,
                .properties = {{css::PropertyId::Color, "blue"s}},
                .children{
                        style::StyledNode{
                                .node{dom_node},
                                .properties{{css::PropertyId::Color, "initial"s}},
                        },
                },
        };

        auto &child = root.children[0];
        child.parent = &root;

        expect_eq(child.get_property(css::PropertyId::Color), "canvastext"sv);
    });

    etest::test("inherit css keyword", [] {
        dom::Node dom_node = dom::Element{"dummy"s};
        style::StyledNode root{
                .node = dom_node,
                .properties = {{css::PropertyId::BackgroundColor, "blue"s}},
                .children{
                        style::StyledNode{
                                .node{dom_node},
                                .properties{
                                        {css::PropertyId::BackgroundColor, "inherit"s},
                                        {css::PropertyId::Width, "inherit"s},
                                },
                        },
                },
        };

        auto &child = root.children[0];
        child.parent = &root;

        // inherit, but not in parent, so receives initial value for property.
        expect_eq(child.get_property(css::PropertyId::Width), "auto"sv);

        // inherit, value in parent.
        expect_eq(child.get_property(css::PropertyId::BackgroundColor), "blue"sv);

        // inherit, no parent node.
        child.parent = nullptr;
        expect_eq(child.get_property(css::PropertyId::BackgroundColor), "transparent");
    });

    etest::test("currentcolor css keyword", [] {
        dom::Node dom_node = dom::Element{"dummy"s};
        style::StyledNode root{
                .node = dom_node,
                .properties = {{css::PropertyId::Color, "blue"s}},
                .children{
                        style::StyledNode{
                                .node{dom_node},
                                .properties{
                                        {css::PropertyId::BackgroundColor, "currentcolor"s},
                                },
                        },
                },
        };

        auto &child = root.children[0];
        child.parent = &root;

        expect_eq(child.get_property(css::PropertyId::BackgroundColor), "blue"sv);

        // "color: currentcolor" should be treated as inherit.
        child.properties.push_back({css::PropertyId::Color, "currentcolor"s});
        expect_eq(child.get_property(css::PropertyId::Color), "blue"sv);
    });

    return etest::run_all_tests();
}
