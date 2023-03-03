// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "layout/layout.h"

#include "etest/etest.h"

using namespace std::literals;
using etest::expect_eq;

int main() {
    etest::test("border radius", [] {
        dom::Node html_node = dom::Element{"html"s};
        style::StyledNode styled_node{
                .node = html_node,
                .properties{
                        {css::PropertyId::FontSize, "30px"},
                        {css::PropertyId::BorderTopLeftRadius, "2em"},
                        {css::PropertyId::BorderBottomRightRadius, "10px/3em"},
                },
        };
        auto layout = layout::create_layout(styled_node, 123).value();

        expect_eq(layout.get_property<css::PropertyId::BorderTopLeftRadius>(), std::pair{60, 60});
        expect_eq(layout.get_property<css::PropertyId::BorderTopRightRadius>(), std::pair{0, 0});
        expect_eq(layout.get_property<css::PropertyId::BorderBottomLeftRadius>(), std::pair{0, 0});
        expect_eq(layout.get_property<css::PropertyId::BorderBottomRightRadius>(), std::pair{10, 90});
    });

    return etest::run_all_tests();
}
