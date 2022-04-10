// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef DOM2_CDATA_SECTION_H_
#define DOM2_CDATA_SECTION_H_

#include "dom2/text.h"

namespace dom2 {

// https://dom.spec.whatwg.org/#interface-cdatasection
class CdataSection final : public Text {
public:
    [[nodiscard]] NodeType type() const override { return NodeType::CdataSection; }
};

} // namespace dom2

#endif
