// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef DOM2_NODE_H_
#define DOM2_NODE_H_

#include <cstdint>
#include <exception>
#include <memory>
#include <vector>

namespace dom2 {

enum class NodeType : std::uint16_t {
    Element = 1,
    Attribute = 2,
    Text = 3,
    CdataSection = 4,
    EntityReference = 5,
    Entity = 6,
    ProcessingInstruction = 7,
    Comment = 8,
    Document = 9,
    DocumentType = 10,
    DocumentFragment = 11,
    Notation = 12,
};

// https://dom.spec.whatwg.org/#interface-node
class Node {
public:
    virtual ~Node() = default;

    [[nodiscard]] virtual NodeType type() const = 0;

    [[nodiscard]] bool has_child_nodes() const { return !child_nodes_.empty(); }
    [[nodiscard]] std::vector<std::shared_ptr<Node>> const &child_nodes() const { return child_nodes_; }
    [[nodiscard]] Node const *first_child() const {
        return child_nodes().empty() ? nullptr : child_nodes().front().get();
    }
    [[nodiscard]] Node const *last_child() const {
        return child_nodes().empty() ? nullptr : child_nodes().back().get();
    }
    [[nodiscard]] Node const *previous_sibling() const { std::terminate(); }
    [[nodiscard]] Node const *next_sibling() const { std::terminate(); }

    std::shared_ptr<Node> append_child(std::shared_ptr<Node> child);

    [[nodiscard]] bool operator==(Node const &) const = default;

private:
    std::vector<std::shared_ptr<Node>> child_nodes_{};

    std::shared_ptr<Node> pre_insert(std::shared_ptr<Node> node, Node const *child);
    void insert(std::shared_ptr<Node> node, Node const *child, bool suppress_observers = false);
};

} // namespace dom2

#endif
