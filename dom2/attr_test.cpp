// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "dom2/attr.h"

#include "etest/etest.h"

using etest::expect_eq;

using namespace dom2;

int main() {
    etest::test("type", [] {
        Attr node{};
        expect_eq(node.type(), NodeType::Attribute);
    });

    return etest::run_all_tests();
}
