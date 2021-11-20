// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef DOM2_SHADOW_ROOT_H_
#define DOM2_SHADOW_ROOT_H_

#include "dom2/document_fragment.h"

namespace dom2 {

// https://dom.spec.whatwg.org/#interface-shadowroot
class ShadowRoot final : public DocumentFragment {};

} // namespace dom2

#endif
