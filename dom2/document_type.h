// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef DOM2_DOCUMENT_TYPE_H_
#define DOM2_DOCUMENT_TYPE_H_

#include "dom2/node.h"

#include <string>
#include <utility>

namespace dom2 {

// https://dom.spec.whatwg.org/#interface-documenttype
class DocumentType final : public Node {
public:
    explicit DocumentType(
            std::string name, std::string public_id = std::string{""}, std::string system_id = std::string{""})
        : name_{std::move(name)}, public_id_{std::move(public_id)}, system_id_{std::move(system_id)} {}

    [[nodiscard]] NodeType type() const override { return NodeType::DocumentType; }

    [[nodiscard]] std::string const &name() const { return name_; }
    [[nodiscard]] std::string const &public_id() const { return public_id_; }
    [[nodiscard]] std::string const &system_id() const { return system_id_; }

private:
    std::string name_{};
    std::string public_id_{};
    std::string system_id_{};
};

} // namespace dom2

#endif
