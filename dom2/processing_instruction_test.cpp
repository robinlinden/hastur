// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "dom2/processing_instruction.h"

#include "etest/etest.h"

#include <string_view>

using namespace std::literals;
using etest::expect_eq;

using namespace dom2;

int main() {
    etest::test("type", [] {
        ProcessingInstruction node{};
        expect_eq(node.type(), NodeType::ProcessingInstruction);
    });

    etest::test("target", [] {
        ProcessingInstruction node{};
        expect_eq(node.data(), ""sv);
        expect_eq(node.target(), ""sv);
    });

    return etest::run_all_tests();
}
