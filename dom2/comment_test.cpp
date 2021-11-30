// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "dom2/comment.h"

#include "etest/etest.h"

#include <string_view>

using namespace std::literals;
using etest::expect_eq;

using namespace dom2;

int main() {
    etest::test("construction", [] {
        Comment node{};
        expect_eq(node.data(), ""sv);
        node = Comment{"comment"};
        expect_eq(node.data(), "comment"sv);
    });

    etest::test("type", [] {
        Comment node{};
        expect_eq(node.type(), NodeType::Comment);
    });

    return etest::run_all_tests();
}
