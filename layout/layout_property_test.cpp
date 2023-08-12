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
    etest::test("get_property", [] {
        dom::Node dom_root = dom::Element{.name{"html"}};
        auto style_root = style::StyledNode{.node = dom_root, .properties = {{css::PropertyId::Color, "green"}}};

        auto layout = layout::create_layout(style_root, 0).value();
        expect_eq(layout.get_property<css::PropertyId::Color>(), gfx::Color::from_css_name("green"));
        expect_eq(layout.get_property<css::PropertyId::BackgroundColor>(), gfx::Color::from_css_name("transparent"));
    });

    using enum css::PropertyId;
    etest::test("border radius", [] {
        expect_property_eq<BorderTopLeftRadius>("2em", std::pair{60, 60}, {{FontSize, "30px"}});
        expect_property_eq<BorderTopRightRadius>(std::nullopt, std::pair{0, 0});
        expect_property_eq<BorderBottomLeftRadius>(std::nullopt, std::pair{0, 0});
        expect_property_eq<BorderBottomRightRadius>("10px/3em", std::pair{10, 90}, {{FontSize, "30px"}});
    });

    etest::test("width", [] {
        expect_property_eq<MinWidth>("13px", 13);
        expect_property_eq<MinWidth>("auto", std::nullopt);

        expect_property_eq<Width>("42px", 42);
        expect_property_eq<Width>("auto", std::nullopt);

        expect_property_eq<MaxWidth>("420px", 420);
        expect_property_eq<MaxWidth>("none", std::nullopt);
    });

    return etest::run_all_tests();
}
