// SPDX-FileCopyrightText: 2021-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "dom/dom.h"

#include "etest/etest2.h"

#include <string_view>

using namespace std::literals;

using dom::Element;

int main() {
    etest::Suite s{"dom"};

    s.add_test("to_string(Document)", [](etest::IActions &a) {
        auto document = dom::Document{.doctype{"html5"}};
        document.html_node = dom::Element{.name{"span"}, .children{{dom::Text{"hello"}}}};
        auto expected = "doctype: html5\ntag: span\n  value: hello\n"sv;
        a.expect_eq(to_string(document), expected);
    });

    s.add_test("to_string(Node)", [](etest::IActions &a) {
        dom::Node root = dom::Element{.name{"span"}, .children{{dom::Text{"hello"}}}};
        auto expected = "tag: span\n  value: hello\n"sv;
        a.expect_eq(to_string(root), expected);
    });

    return s.run();
}
