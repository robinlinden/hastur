// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef DOM2_DOCUMENT_FRAGMENT_H_
#define DOM2_DOCUMENT_FRAGMENT_H_

#include "dom2/node.h"

namespace dom2 {

// https://dom.spec.whatwg.org/#interface-documentfragment
class DocumentFragment : public Node {
public:
    NodeType type() const final { return NodeType::DocumentFragment; }
};

} // namespace dom2

#endif
