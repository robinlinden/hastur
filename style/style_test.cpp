// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "style/style.h"
#include "style/styled_node.h"

#include "css/rule.h"
#include "etest/etest.h"

using namespace std::literals;
using etest::expect;
using etest::require;

// TODO(robinlinden): clang-format doesn't get along well with how I structured
// the trees in these test cases.

// clang-format off

int main() {
    etest::test("is_match: simple names", [] {
        expect(style::is_match(dom::Element{"div"}, "div"sv));
        expect(!style::is_match(dom::Element{"div"}, "span"sv));
    });

    etest::test("matching_rules: simple names", [] {
        std::vector<css::Rule> stylesheet;
        expect(style::matching_rules(dom::Element{"div"}, stylesheet).empty());

        stylesheet.push_back(css::Rule{
            .selectors = {"span", "p"},
            .declarations = {
                {"width", "80px"},
            }
        });

        expect(style::matching_rules(dom::Element{"div"}, stylesheet).empty());

        {
            auto span_rules = style::matching_rules(dom::Element{"span"}, stylesheet);
            require(span_rules.size() == 1);
            expect(span_rules[0] == std::pair{"width"s, "80px"s});
        }

        {
            auto p_rules = style::matching_rules(dom::Element{"p"}, stylesheet);
            require(p_rules.size() == 1);
            expect(p_rules[0] == std::pair{"width"s, "80px"s});
        }

        stylesheet.push_back(css::Rule{
            .selectors = {"span", "hr"},
            .declarations = {
                {"height", "auto"},
            }
        });

        expect(style::matching_rules(dom::Element{"div"}, stylesheet).empty());

        {
            auto span_rules = style::matching_rules(dom::Element{"span"}, stylesheet);
            require(span_rules.size() == 2);
            expect(span_rules[0] == std::pair{"width"s, "80px"s});
            expect(span_rules[1] == std::pair{"height"s, "auto"s});
        }

        {
            auto p_rules = style::matching_rules(dom::Element{"p"}, stylesheet);
            require(p_rules.size() == 1);
            expect(p_rules[0] == std::pair{"width"s, "80px"s});
        }

        {
            auto hr_rules = style::matching_rules(dom::Element{"hr"}, stylesheet);
            require(hr_rules.size() == 1);
            expect(hr_rules[0] == std::pair{"height"s, "auto"s});
        }
    });

    etest::test("style_tree: structure", [] {
        dom::Node root = dom::create_element_node(
            "html",
            {},
            {
                dom::create_element_node("head", {}, {}),
                dom::create_element_node("body", {}, {
                    dom::create_element_node("p", {}, {}),
                }),
            }
        );

        style::StyledNode expected{
            .node = root,
            .properties = {},
            .children = {
                {root.children[0], {}, {}},
                {root.children[1], {}, {
                    {root.children[1].children[0], {}, {}},
                }},
            },
        };

        expect(style::style_tree(root, {}) == expected);
    });

    etest::test("style_tree: style is applied", [] {
        dom::Node root = dom::create_element_node(
            "html",
            {},
            {
                dom::create_element_node("head", {}, {}),
                dom::create_element_node("body", {}, {
                    dom::create_element_node("p", {}, {}),
                }),
            }
        );

        std::vector<css::Rule> stylesheet{
            {
                .selectors = {"p"},
                .declarations = {
                    {"height", "100px"},
                }
            },
            {
                .selectors = {"body"},
                .declarations = {
                    {"text-size", "500em"},
                }
            },
        };

        style::StyledNode expected{
            .node = root,
            .properties = {},
            .children = {
                {root.children[0], {}, {}},
                {root.children[1], {{"text-size", "500em"}}, {
                    {root.children[1].children[0], {{"height", "100px"}}, {}},
                }},
            },
        };

        expect(style::style_tree(root, stylesheet) == expected);
    });

    return etest::run_all_tests();
}
