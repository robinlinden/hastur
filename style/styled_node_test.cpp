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

        expect_eq(style::get_property(child, "width"sv), std::nullopt);
        expect_eq(style::get_property(child, "font-size"sv), "15em"sv);
    });

    return etest::run_all_tests();
}
