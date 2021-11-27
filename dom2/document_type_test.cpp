// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "dom2/document_type.h"

#include "etest/etest.h"

#include <string>
#include <string_view>

using namespace std::literals;
using etest::expect;
using etest::expect_eq;

using namespace dom2;

int main() {
    etest::test("type", [] {
        DocumentType node{"html"s};
        expect_eq(node.type(), NodeType::DocumentType);
    });

    etest::test("construction", [] {
        DocumentType node{"html"s};
        expect_eq(node.name(), "html"sv);
        expect(node.public_id().empty());
        expect(node.system_id().empty());

        DocumentType node_with_public_id{"good name"s, "some id"s};
        expect_eq(node_with_public_id.name(), "good name"sv);
        expect_eq(node_with_public_id.public_id(), "some id"sv);
        expect(node_with_public_id.system_id().empty());

        DocumentType node_with_public_and_system_id{"good name"s, "some id"s, "another id"};
        expect_eq(node_with_public_and_system_id.name(), "good name"sv);
        expect_eq(node_with_public_and_system_id.public_id(), "some id"sv);
        expect_eq(node_with_public_and_system_id.system_id(), "another id"sv);
    });

    return etest::run_all_tests();
}
