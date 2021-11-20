// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef DOM2_DOCUMENT_TYPE_H_
#define DOM2_DOCUMENT_TYPE_H_

#include "dom2/node.h"

namespace dom2 {

// https://dom.spec.whatwg.org/#interface-documenttype
class DocumentType final : public Node {
public:
    NodeType type() const override { return NodeType::DocumentType; }
};

} // namespace dom2

#endif
