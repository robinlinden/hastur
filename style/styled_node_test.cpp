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
        dom::Node dom_node = dom::create_element_node("dummy"sv, {}, {});
        style::StyledNode styled_node{
                .node = dom_node,
                .properties = {{"good_property"s, "fantastic_value"s}},
                .children = {},
        };

        expect(style::get_property(styled_node, "bad_property"sv) == std::nullopt);
        expect(style::get_property(styled_node, "good_property"sv).value() == "fantastic_value"sv);
    });

    etest::test("property inheritance", [] {
        dom::Node dom_node = dom::create_element_node("dummy"sv, {}, {});
        style::StyledNode root{
                .node = dom_node,
                .properties = {{"font-size"s, "15em"s}, {"width"s, "0px"s}},
                .children = {},
        };

        auto &child = root.children.emplace_back(style::StyledNode{dom_node, {}, {}, &root});

        // Not inherited, returns the initial value.
        expect_eq(style::get_property(child, "width"sv), "auto"sv);

        // Inherited, returns the parent's value.
        expect_eq(style::get_property(child, "font-size"sv), "15em"sv);
    });

    etest::test("inherit css keyword", [] {
        dom::Node dom_node = dom::create_element_node("dummy"sv, {}, {});
        style::StyledNode root{
                .node = dom_node,
                .properties = {{"background-color"s, "blue"s}},
                .children{
                        style::StyledNode{
                                .node{dom_node},
                                .properties{
                                        {"background-color"s, "inherit"s},
                                        {"width"s, "inherit"s},
                                },
                        },
                },
        };

        auto &child = root.children[0];
        child.parent = &root;

        // inherit, but not in parent, so receives initial value for property.
        expect_eq(style::get_property(child, "width"sv), "auto"sv);

        // inherit, value in parent.
        expect_eq(style::get_property(child, "background-color"sv), "blue"sv);

        // inherit, no parent node.
        child.parent = nullptr;
        expect_eq(style::get_property(child, "background-color"sv), std::nullopt);
    });

    etest::test("currentcolor css keyword", [] {
        dom::Node dom_node = dom::create_element_node("dummy"sv, {}, {});
        style::StyledNode root{
                .node = dom_node,
                .properties = {{"color"s, "blue"s}},
                .children{
                        style::StyledNode{
                                .node{dom_node},
                                .properties{
                                        {"background-color"s, "currentcolor"s},
                                },
                        },
                },
        };

        auto &child = root.children[0];
        child.parent = &root;

        expect_eq(style::get_property(child, "background-color"sv), "blue"sv);

        // "color: currentcolor" should be treated as inherit.
        child.properties.push_back({"color"s, "currentcolor"s});
        expect_eq(style::get_property(child, "color"sv), "blue"sv);
    });

    return etest::run_all_tests();
}
