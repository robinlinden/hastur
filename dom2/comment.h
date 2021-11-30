// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef DOM2_COMMENT_H_
#define DOM2_COMMENT_H_

#include "dom2/character_data.h"

#include <string>
#include <utility>

namespace dom2 {

// https://dom.spec.whatwg.org/#interface-comment
class Comment final : public CharacterData {
public:
    explicit Comment(std::string data = std::string{""}) : CharacterData(std::move(data)) {}

    NodeType type() const override { return NodeType::Comment; }
};

} // namespace dom2

#endif
