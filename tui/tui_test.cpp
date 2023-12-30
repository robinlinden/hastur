// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "tui/tui.h"

#include "css/property_id.h"
#include "css/rule.h"
#include "dom/dom.h"
#include "etest/etest2.h"
#include "layout/layout.h"
#include "style/style.h"
#include "util/string.h"

// There will be some bonus util::trim calls in these tests for now as FTXUI
// adds a lot of whitespace around the rendered layout.
int main() {
    etest::Suite s{"tui"};

    s.add_test("Text in block", [](etest::IActions &a) {
        dom::Node dom{dom::Element{"div", {}, {dom::Text{"Hello, world!"}}}};
        auto style = style::style_tree(dom, {{css::Rule{{"div"}, {{css::PropertyId::Display, "block"}}}}});
        auto layout = layout::create_layout(*style, 9000);
        auto rendered = tui::render(layout.value());
        a.expect_eq(util::trim(rendered), "Hello, world!");
    });

    s.add_test("Whitespace-collapsing", [](etest::IActions &a) {
        dom::Node dom{dom::Element{"div", {}, {dom::Text{"Hello,              world!"}}}};
        auto style = style::style_tree(dom, {{css::Rule{{"div"}, {{css::PropertyId::Display, "block"}}}}});
        auto layout = layout::create_layout(*style, 9000);
        auto rendered = tui::render(layout.value());
        a.expect_eq(util::trim(rendered), "Hello, world!");
    });

    return s.run();
}
