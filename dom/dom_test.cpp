// SPDX-FileCopyrightText: 2021-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "dom/dom.h"

#include "etest/etest2.h"

#include <string_view>

int main() {
    etest::Suite s{"dom"};

    s.add_test("to_string(Document)", [](etest::IActions &a) {
        auto document = dom::Document{.doctype{"html5"}};
        document.html_node = dom::Element{
                .name{"span"},
                .children{{
                        dom::Text{"hello"},
                        dom::Element{
                                .name{"a"},
                                .attributes{{"href", "https://example.com"}, {"class", "link"}},
                                .children{dom::Text{"go!"}},
                        },
                }},
        };

        std::string_view expected =
                "#document\n"
                "| <!DOCTYPE html5>\n"
                "| <span>\n"
                "|   \"hello\"\n"
                "|   <a>\n"
                "|     class=\"link\"\n"
                "|     href=\"https://example.com\"\n"
                "|     \"go!\"";
        a.expect_eq(to_string(document), expected);
    });

    s.add_test("to_string(Node)", [](etest::IActions &a) {
        dom::Node root = dom::Element{.name{"span"}, .children{{dom::Text{"hello"}}}};
        std::string_view expected =
                "<span>\n"
                "| \"hello\"";
        a.expect_eq(to_string(root), expected);
    });

    return s.run();
}
