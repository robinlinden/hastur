// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "style/styled_node.h"

#include "etest/etest.h"

#include <tuple>

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

        expect(styled_node.get_property<css::PropertyId::Width>() == "15px"sv);
    });

    etest::test("property inheritance", [] {
        dom::Node dom_node = dom::Element{"dummy"s};
        style::StyledNode root{
                .node = dom_node,
                .properties = {{css::PropertyId::FontSize, "15px"s}, {css::PropertyId::Width, "0px"s}},
                .children = {},
        };

        auto &child = root.children.emplace_back(style::StyledNode{dom_node, {}, {}, &root});

        // Not inherited, returns the initial value.
        expect_eq(child.get_property<css::PropertyId::Width>(), "auto"sv);

        // Inherited, returns the parent's value.
        expect_eq(child.get_property<css::PropertyId::FontSize>(), 15);
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

        expect_eq(child.get_property<css::PropertyId::Color>(), "canvastext"sv);
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
        expect_eq(child.get_property<css::PropertyId::Width>(), "auto"sv);

        // inherit, value in parent.
        expect_eq(child.get_property<css::PropertyId::BackgroundColor>(), "blue"sv);

        // inherit, no parent node.
        child.parent = nullptr;
        expect_eq(child.get_property<css::PropertyId::BackgroundColor>(), "transparent");
    });

    etest::test("unset css keyword", [] {
        dom::Node dom_node = dom::Element{"dummy"s};
        style::StyledNode root{
                .node = dom_node,
                .properties = {{css::PropertyId::Color, "blue"s}},
                .children{
                        style::StyledNode{
                                .node{dom_node},
                                .properties{
                                        {css::PropertyId::Color, "unset"s},
                                        {css::PropertyId::Width, "unset"s},
                                },
                        },
                },
        };

        auto &child = root.children[0];
        child.parent = &root;

        // unset, not inherited, so receives initial value for property.
        expect_eq(child.get_property<css::PropertyId::Width>(), "auto"sv);

        // unset, inherited, value in parent.
        expect_eq(child.get_property<css::PropertyId::Color>(), "blue"sv);

        // unset, inherited, no parent node.
        child.parent = nullptr;
        expect_eq(child.get_property<css::PropertyId::Color>(), "canvastext");
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

        expect_eq(child.get_property<css::PropertyId::BackgroundColor>(), "blue"sv);

        // "color: currentcolor" should be treated as inherit.
        child.properties.push_back({css::PropertyId::Color, "currentcolor"s});
        expect_eq(child.get_property<css::PropertyId::Color>(), "blue"sv);
    });

    etest::test("get_font_style_property", [] {
        dom::Node dom_node = dom::Element{"dummy"s};
        style::StyledNode styled_node{.node = dom_node, .properties = {{css::PropertyId::FontStyle, "oblique"s}}};

        expect_eq(styled_node.get_property<css::PropertyId::FontStyle>(), style::FontStyle::Oblique);

        // Unhandled properties don't break things.
        styled_node.properties[0] = std::pair{css::PropertyId::FontStyle, "???"s};
        expect_eq(styled_node.get_property<css::PropertyId::FontStyle>(), style::FontStyle::Normal);
    });

    etest::test("get_font_family_property", [] {
        dom::Node dom_node = dom::Element{"dummy"s};
        style::StyledNode styled_node{.node = dom_node, .properties = {{css::PropertyId::FontFamily, "abc, def"s}}};
        expect_eq(styled_node.get_property<css::PropertyId::FontFamily>(), std::vector<std::string_view>{"abc", "def"});
    });

    etest::test("get_font_size_property", [] {
        dom::Node dom_node = dom::Element{"dummy"s};
        style::StyledNode root{
                .node = dom_node,
                .properties = {{css::PropertyId::FontSize, "50px"s}},
                .children{
                        style::StyledNode{
                                .node{dom_node},
                                .properties{{css::PropertyId::FontSize, "10px"s}},
                        },
                },
        };

        auto &child = root.children[0];
        child.parent = &root;

        // px
        expect_eq(child.get_property<css::PropertyId::FontSize>(), 10);
        expect_eq(root.get_property<css::PropertyId::FontSize>(), 50);

        // em
        // TODO(robinlinden): Not correct, but this is the behaviour we had in //layout previously.
        child.properties[0] = {css::PropertyId::FontSize, "2em"};
        // TODO(robinlinden): Should be 100.
        expect_eq(child.get_property<css::PropertyId::FontSize>(), 20);

        // unhandled units
        // TODO(robinlinden): We should probably return 0 or something for this,
        //                    but this matches the behaviour from //layout.
        child.properties[0] = {css::PropertyId::FontSize, "1asdf"};
        expect_eq(child.get_property<css::PropertyId::FontSize>(), 1);

        // 0
        child.properties[0] = {css::PropertyId::FontSize, "0"};
        expect_eq(child.get_property<css::PropertyId::FontSize>(), 0);

        // Invalid, shouldn't crash.
        // TODO(robinlinden): Make this do whatever other browsers do.
        child.properties[0] = {css::PropertyId::FontSize, "abcd"};
        std::ignore = child.get_property<css::PropertyId::FontSize>();
    });

    return etest::run_all_tests();
}
