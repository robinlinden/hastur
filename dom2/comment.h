// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef DOM2_COMMENT_H_
#define DOM2_COMMENT_H_

#include "dom2/character_data.h"

namespace dom2 {

// https://dom.spec.whatwg.org/#interface-comment
class Comment final : public CharacterData {
public:
    NodeType type() const override { return NodeType::Comment; }
};

} // namespace dom2

#endif
