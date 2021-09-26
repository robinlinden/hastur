// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "style/styled_node.h"

#include "etest/etest.h"

using namespace std::literals;
using etest::expect;
using etest::require;

int main() {
    etest::test("get_property"sv, [] {
        dom::Node dom_node = dom::create_element_node("dummy"sv, {}, {});
        style::StyledNode styled_node{
                .node = dom_node,
                .properties = {{"good_property"s, "fantastic_value"s}},
                .children = {},
        };

        expect(style::get_property(styled_node, "bad_property"sv) == std::nullopt);
        expect(style::get_property(styled_node, "good_property"sv).value() == "fantastic_value"sv);
        expect(style::get_property_or(styled_node, "bad_property"sv, "fallback"sv) == "fallback"sv);
        expect(style::get_property_or(styled_node, "good_property"sv, "fallback"sv) == "fantastic_value"sv);
    });

    return etest::run_all_tests();
}
