// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "css/rule.h"

#include "css/media_query.h"
#include "css/property_id.h"

#include "etest/etest2.h"

int main() {
    etest::Suite s{};
    s.add_test("rule to string, one selector and declaration", [](etest::IActions &a) {
        css::Rule rule;
        rule.selectors.emplace_back("div");
        rule.declarations.emplace(css::PropertyId::BackgroundColor, "black");

        auto const *expected =
                "Selectors: div\n"
                "Declarations:\n"
                "  background-color: black\n";
        a.expect_eq(css::to_string(rule), expected);
    });

    s.add_test("rule to string, two selectors and several declarations", [](etest::IActions &a) {
        css::Rule rule;
        rule.selectors.emplace_back("h1");
        rule.selectors.emplace_back("h2");
        rule.declarations.emplace(css::PropertyId::Color, "blue");
        rule.declarations.emplace(css::PropertyId::FontFamily, "Arial");
        rule.declarations.emplace(css::PropertyId::TextAlign, "center");

        auto const *expected =
                "Selectors: h1, h2\n"
                "Declarations:\n"
                "  color: blue\n"
                "  font-family: Arial\n"
                "  text-align: center\n";
        a.expect_eq(css::to_string(rule), expected);
    });

    s.add_test("rule to string, two selectors and several declarations", [](etest::IActions &a) {
        css::Rule rule;
        rule.selectors.emplace_back("h1");
        rule.declarations.emplace(css::PropertyId::Color, "blue");
        rule.declarations.emplace(css::PropertyId::TextAlign, "center");
        rule.media_query = css::MediaQuery{css::MediaQuery::Width{.max = 900}};

        auto const *expected =
                "Selectors: h1\n"
                "Declarations:\n"
                "  color: blue\n"
                "  text-align: center\n"
                "Media query:\n"
                "  0 <= width <= 900\n";
        a.expect_eq(css::to_string(rule), expected);
    });

    s.add_test("rule to string, important declaration", [](etest::IActions &a) {
        css::Rule rule;
        rule.selectors.emplace_back("div");
        rule.important_declarations.emplace(css::PropertyId::BackgroundColor, "black");

        auto const *expected =
                "Selectors: div\n"
                "Declarations:\n"
                "Important declarations:\n"
                "  background-color: black\n";
        a.expect_eq(css::to_string(rule), expected);
    });

    s.add_test("rule to string, custom property", [](etest::IActions &a) {
        css::Rule rule;
        rule.selectors.emplace_back("div");
        rule.custom_properties.emplace("--ping", "pong");

        auto const *expected =
                "Selectors: div\n"
                "Declarations:\n"
                "Custom properties:\n"
                "  --ping: pong\n";
        a.expect_eq(css::to_string(rule), expected);
    });

    return s.run();
}
