// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "style/style.h"

#include "style/styled_node.h"

#include "dom/dom.h"
#include "etest/etest2.h"

#include <nanobench.h>

int main() {
    etest::Suite s;

    s.add_test("is_match: class", [](etest::IActions const &) {
        ankerl::nanobench::Bench bench;
        bench.title("is_match: class");

        dom::Node few_classes_dom = dom::Element{"div", {{"class", "first second"}}};
        auto few_classes = style::StyledNode{.node = few_classes_dom};
        bench.run("match, few classes", [&] {
            style::is_match(few_classes, ".first.second"); //
        });

        bench.run("no match, few classes", [&] {
            style::is_match(few_classes, ".first.second.third.fourth"); //
        });

        dom::Node many_classes_dom = dom::Element{
                "div",
                {{"class", "one two three four five six seven eight nine ten"}},
        };
        auto many_classes = style::StyledNode{.node = many_classes_dom};
        bench.run("match, many classes", [&] {
            style::is_match(many_classes, ".eight.two.seven.ten"); //
        });

        bench.run("no match, many classes", [&] {
            style::is_match(many_classes, ".eight.two.seve.ten"); //
        });
    });

    return s.run();
}
