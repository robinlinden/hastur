// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef DOM2_ELEMENT_H_
#define DOM2_ELEMENT_H_

#include "dom2/node.h"

#include <string>
#include <utility>

namespace dom2 {

// https://dom.spec.whatwg.org/#interface-element
// TODO(robinlinden): This is only partially implemented.
class Element final : public Node {
public:
    explicit Element(std::string local_name) : local_name_{std::move(local_name)} {}

    [[nodiscard]] NodeType type() const override { return NodeType::Element; }

    [[nodiscard]] std::string const &local_name() const { return local_name_; }

private:
    std::string local_name_{};
};

} // namespace dom2

#endif
