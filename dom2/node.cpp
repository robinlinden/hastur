// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "dom2/node.h"

#include <exception>
#include <utility>

namespace dom2 {

// https://dom.spec.whatwg.org/#concept-node-append
std::shared_ptr<Node> Node::append_child(std::shared_ptr<Node> child) {
    // To append a node to a parent, pre-insert node into parent before null.
    return pre_insert(std::move(child), nullptr);
}

// https://dom.spec.whatwg.org/#concept-node-pre-insert
std::shared_ptr<Node> Node::pre_insert(std::shared_ptr<Node> node, Node const *child) {
    // 1. Ensure pre-insertion validity of node into parent before child.
    // TODO(robinlinden): Ensure pre-insertion validity.

    // 2. Let referenceChild be child.
    auto const *reference_child = child;

    // 3. If referenceChild is node, then set referenceChild to node's next sibling.
    if (reference_child == node.get()) {
        reference_child = node->next_sibling();
    }

    // 4. Insert node into parent before referenceChild.
    insert(node, reference_child);

    // 5. Return node.
    return node;
}

// https://dom.spec.whatwg.org/#concept-node-insert
void Node::insert(std::shared_ptr<Node> node, Node const *child, [[maybe_unused]] bool suppress_observers) {
    // 1. Let nodes be node's children, if node is a DocumentFragment node; otherwise « node ».
    auto const &nodes =
            node->type() == NodeType::DocumentFragment ? node->child_nodes() : std::vector<std::shared_ptr<Node>>{node};

    // 2. Let count be nodes's size.
    auto count = nodes.size();

    // 3. If count is 0, then return.
    if (count == 0) {
        return;
    }

    // 4. If node is a DocumentFragment node, then:
    if (node->type() == NodeType::DocumentFragment) {
        // 1. Remove its children with the suppress observers flag set.
        // 2. Queue a tree mutation record for node with « », nodes, null, and null.
        // Note: This step intentionally does not pay attention to the suppress observers flag.
        std::terminate();
    }

    // 5. If child is non-null, then:
    if (child != nullptr) {
        // 1. For each live range whose start node is parent and start offset is greater than child's index, increase
        // its start offset by count.
        // 2. For each live range whose end node is parent and end offset is greater than child's index, increase its
        // end offset by count.
        std::terminate();
    }

    // 6. Let previousSibling be child's previous sibling or parent's last child if child is null.
    [[maybe_unused]] Node const *previous_sibling = child != nullptr ? child->previous_sibling() : last_child();

    // 7. For each node in nodes, in tree order:
    for (auto const &n : nodes) {
        // TODO(robinlinden):
        // 1. Adopt node into parent's node document.

        // 2. If child is null, then append node to parent's children.
        // 3. Otherwise, insert node into parent's children before child's index.
        if (child == nullptr) {
            child_nodes_.push_back(n);
        } else {
            std::terminate();
        }

        // Please keep my indented comments, clang-format. It looks silly, but they go where blocks will go.
        // clang-format off
        // TODO(robinlinden):
        // 4. If parent is a shadow host whose shadow root's slot assignment is "named" and node is a slottable, then
        // assign a slot for node.
        // 5. If parent's root is a shadow root, and parent is a slot whose assigned nodes is the empty list, then run
        // signal a slot change for parent.
        // 6. Run assign slottables for a tree with node's root.
        // 7. For each shadow-including inclusive descendant inclusiveDescendant of node, in shadow-including tree
        // order:
            // 1. Run the insertion steps with inclusiveDescendant.
            // 2. If inclusiveDescendant is connected, then:
                // 1. If inclusiveDescendant is custom, then enqueue a custom element callback reaction with
                // inclusiveDescendant, callback name "connectedCallback", and an empty argument list.
                // 2. Otherwise, try to upgrade inclusiveDescendant.
                // Note: If this successfully upgrades inclusiveDescendant, its connectedCallback will be enqueued
                // automatically during the upgrade an element algorithm.
    }

    // TODO(robinlinden):
    // 8. If suppress observers flag is unset, then queue a tree mutation record for parent with nodes, « »,
    // previousSibling, and child.
    // 9. Run the children changed steps for parent.
}

} // namespace dom2
