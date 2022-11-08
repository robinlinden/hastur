// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "css/rule.h"

#include "css/property_id.h"

#include "etest/etest.h"

using etest::expect_eq;

int main() {
    etest::test("rule to string, one selector and declaration", [] {
        css::Rule rule;
        rule.selectors.emplace_back("div");
        rule.declarations.emplace(css::PropertyId::BackgroundColor, "black");

        auto expected =
                "Selectors: div\n"
                "Declarations:\n"
                "  background-color: black\n";
        expect_eq(css::to_string(rule), expected);
    });

    etest::test("rule to string, two selectors and several declarations", [] {
        css::Rule rule;
        rule.selectors.emplace_back("h1");
        rule.selectors.emplace_back("h2");
        rule.declarations.emplace(css::PropertyId::Color, "blue");
        rule.declarations.emplace(css::PropertyId::FontFamily, "Arial");
        rule.declarations.emplace(css::PropertyId::TextAlign, "center");

        auto expected =
                "Selectors: h1, h2\n"
                "Declarations:\n"
                "  color: blue\n"
                "  font-family: Arial\n"
                "  text-align: center\n";
        expect_eq(css::to_string(rule), expected);
    });

    etest::test("rule to string, two selectors and several declarations", [] {
        css::Rule rule;
        rule.selectors.emplace_back("h1");
        rule.declarations.emplace(css::PropertyId::Color, "blue");
        rule.declarations.emplace(css::PropertyId::TextAlign, "center");
        rule.media_query = "screen and (min-width: 900px)";

        auto expected =
                "Selectors: h1\n"
                "Declarations:\n"
                "  color: blue\n"
                "  text-align: center\n"
                "Media query:\n"
                "  screen and (min-width: 900px)\n";
        expect_eq(css::to_string(rule), expected);
    });

    return etest::run_all_tests();
}
