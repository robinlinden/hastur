// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef DOM2_ELEMENT_H_
#define DOM2_ELEMENT_H_

#include "dom2/node.h"

namespace dom2 {

// https://dom.spec.whatwg.org/#interface-element
class Element final : public Node {
public:
    NodeType type() const override { return NodeType::Element; }
};

} // namespace dom2

#endif
