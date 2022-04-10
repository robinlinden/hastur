// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "dom2/node.h"

#include "etest/etest.h"

using namespace std::literals;
using etest::expect;
using etest::expect_eq;
using etest::require;

using namespace dom2;

namespace {
struct TestNode final : Node {
    explicit TestNode(NodeType type) : type_{type} {}
    [[nodiscard]] NodeType type() const override { return type_; }

private:
    NodeType type_;
};
} // namespace

int main() {
    etest::test("append_node", [] {
        TestNode node{NodeType::Document};
        expect_eq(node.child_nodes().size(), static_cast<std::size_t>(0));

        node.append_child(std::make_shared<TestNode>(NodeType::Element));
        expect_eq(node.child_nodes().size(), static_cast<std::size_t>(1));

        node.append_child(std::make_shared<TestNode>(NodeType::Comment));
        expect_eq(node.child_nodes().size(), static_cast<std::size_t>(2));
    });

    etest::test("has_child_nodes", [] {
        TestNode node{NodeType::Document};
        expect(!node.has_child_nodes());
        node.append_child(std::make_shared<TestNode>(NodeType::Element));
        expect(node.has_child_nodes());
    });

    etest::test("first_child", [] {
        TestNode node{NodeType::Document};
        expect_eq(node.first_child(), nullptr);

        node.append_child(std::make_shared<TestNode>(NodeType::Element));
        require(node.first_child() != nullptr);
        expect_eq(node.first_child()->type(), NodeType::Element);

        node.append_child(std::make_shared<TestNode>(NodeType::Comment));
        require(node.first_child() != nullptr);
        expect_eq(node.first_child()->type(), NodeType::Element);
    });

    etest::test("last_child", [] {
        TestNode node{NodeType::Document};
        expect_eq(node.last_child(), nullptr);

        node.append_child(std::make_shared<TestNode>(NodeType::Element));
        require(node.last_child() != nullptr);
        expect_eq(node.last_child()->type(), NodeType::Element);

        node.append_child(std::make_shared<TestNode>(NodeType::Comment));
        require(node.last_child() != nullptr);
        expect_eq(node.last_child()->type(), NodeType::Comment);
    });

    return etest::run_all_tests();
}
