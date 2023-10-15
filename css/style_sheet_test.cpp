// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "css/style_sheet.h"

#include "etest/etest2.h"

#include <utility>
#include <vector>

int main() {
    etest::Suite s;

    s.add_test("StyleSheet::splice", [](etest::IActions &a) {
        css::StyleSheet a1;
        a1.rules.push_back({.selectors = {"a"}});
        a1.rules.push_back({.selectors = {"b"}});
        css::StyleSheet a2;
        a2.rules.push_back({.selectors = {"c"}});
        a2.rules.push_back({.selectors = {"d"}});

        a1.splice(std::move(a2));
        a.expect_eq(a1.rules, std::vector<css::Rule>{{{"a"}}, {{"b"}}, {{"c"}}, {{"d"}}});
    });

    return s.run();
}
