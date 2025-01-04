// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "dom/dom.h"
#include "etest/etest2.h"
#include "style/style.h"
#include "style/styled_node.h"

#include <nanobench.h>

int main() {
    etest::Suite s;

    s.add_test("is_match: class", [](auto const &) {
        ankerl::nanobench::Bench bench;
        dom::Node dom = dom::Element{"div", {{"class", "foo bar baz hello first second"}}};
        auto styled = style::StyledNode{.node = dom};

        bench.run("is_match: class", [&] {
            style::is_match(styled, ".first.second"); //
        });
    });

    return s.run();
}
