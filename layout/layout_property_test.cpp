// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "layout/layout.h"

#include "css/property_id.h"
#include "dom/dom.h"
#include "etest/etest2.h"
#include "gfx/color.h"
#include "style/styled_node.h"
#include "style/unresolved_value.h"

#include <optional>
#include <source_location>
#include <string>
#include <utility>
#include <vector>

using namespace std::literals;

namespace {
template<css::PropertyId IdT>
void expect_property_eq(etest::IActions &a,
        std::optional<std::string> value,
        auto expected,
        std::vector<std::pair<css::PropertyId, std::string>> extra_properties = {},
        std::source_location const &loc = std::source_location::current()) {
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
        a.expect(false, std::nullopt, loc);
        return;
    }

    a.expect_eq(layout->get_property<IdT>(), expected, std::nullopt, loc);
};
} // namespace

int main() {
    etest::Suite s{};

    s.add_test("get_property", [](etest::IActions &a) {
        dom::Node dom_root = dom::Element{.name{"html"}};
        auto style_root = style::StyledNode{.node = dom_root, .properties = {{css::PropertyId::Color, "green"}}};

        auto layout = layout::create_layout(style_root, 0).value();
        a.expect_eq(layout.get_property<css::PropertyId::Color>(), gfx::Color::from_css_name("green"));
        a.expect_eq(layout.get_property<css::PropertyId::BackgroundColor>(), gfx::Color::from_css_name("transparent"));
    });

    using enum css::PropertyId;
    s.add_test("border radius", [](etest::IActions &a) {
        expect_property_eq<BorderTopLeftRadius>(a, "2em", std::pair{60, 60}, {{FontSize, "30px"}});
        expect_property_eq<BorderTopRightRadius>(a, std::nullopt, std::pair{0, 0});
        expect_property_eq<BorderBottomLeftRadius>(a, std::nullopt, std::pair{0, 0});
        expect_property_eq<BorderBottomRightRadius>(a, "10px/3em", std::pair{10, 90}, {{FontSize, "30px"}});
    });

    s.add_test("width", [](etest::IActions &a) {
        expect_property_eq<MinWidth>(a, "13px", style::UnresolvedValue{"13px"});
        expect_property_eq<MinWidth>(a, "auto", style::UnresolvedValue{"auto"});

        expect_property_eq<Width>(a, "42px", style::UnresolvedValue{"42px"});
        expect_property_eq<Width>(a, "auto", style::UnresolvedValue{"auto"});

        expect_property_eq<MaxWidth>(a, "420px", style::UnresolvedValue{"420px"});
        expect_property_eq<MaxWidth>(a, "none", style::UnresolvedValue{"none"});
    });

    return s.run();
}
