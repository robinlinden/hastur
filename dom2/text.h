// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef DOM2_TEXT_H_
#define DOM2_TEXT_H_

#include "dom2/character_data.h"

namespace dom2 {

// https://dom.spec.whatwg.org/#interface-text
class Text : public CharacterData {
public:
    NodeType type() const override { return NodeType::Text; }
};

} // namespace dom2

#endif
