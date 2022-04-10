// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef DOM2_DOCUMENT_H_
#define DOM2_DOCUMENT_H_

#include "dom2/node.h"

namespace dom2 {

// https://dom.spec.whatwg.org/#interface-document
class Document final : public Node {
public:
    [[nodiscard]] NodeType type() const override { return NodeType::Document; }
};

} // namespace dom2

#endif
