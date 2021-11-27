// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "dom2/element.h"

#include "etest/etest.h"

#include <string>

using namespace std::literals;
using etest::expect_eq;

using namespace dom2;

int main() {
    etest::test("type", [] {
        Element node{"a"s};
        expect_eq(node.type(), NodeType::Element);
    });

    etest::test("local name", [] {
        Element node{"title"s};
        expect_eq(node.local_name(), "title");
    });

    return etest::run_all_tests();
}
