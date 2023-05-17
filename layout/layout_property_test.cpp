// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "layout/layout.h"

#include "etest/cxx_compat.h"
#include "etest/etest.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace std::literals;
using etest::expect_eq;

namespace {
template<css::PropertyId IdT>
void expect_property_eq(std::optional<std::string> value,
        auto expected,
        std::vector<std::pair<css::PropertyId, std::string>> extra_properties = {},
        etest::source_location const &loc = etest::source_location::current()) {
    dom::Node dom_node = dom::Element{"dummy"s};
    style::StyledNode styled_node{
            .node = dom_node,
            .properties = std::move(extra_properties),
            .children = {},
    };

    if (value) {
        styled_node.properties.push_back({IdT, *std::move(value)});
    }

    auto layout = layout::create_layout(styled_node, 123);
    if (!layout) {
        etest::expect(false, std::nullopt, loc);
        return;
    }

    etest::expect_eq(layout->get_property<IdT>(), expected, std::nullopt, loc);
};
} // namespace

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
