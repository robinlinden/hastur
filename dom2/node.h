// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef DOM2_NODE_H_
#define DOM2_NODE_H_

#include <cstdint>

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

    virtual NodeType type() const = 0;
};

} // namespace dom2

#endif
