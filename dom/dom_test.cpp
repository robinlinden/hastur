// SPDX-FileCopyrightText: 2021-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "dom/dom.h"

#include "etest/etest.h"

#include <string_view>

using namespace std::literals;

using dom::Element;
using etest::expect_eq;

int main() {
    etest::test("to_string", [] {
        auto document = dom::Document{.doctype{"html5"}};
        document.html_node = dom::Element{.name{"span"}, .children{{dom::Text{"hello"}}}};
        auto expected = "doctype: html5\ntag: span\n  value: hello\n"sv;
        expect_eq(to_string(document), expected);
    });

    return etest::run_all_tests();
}
