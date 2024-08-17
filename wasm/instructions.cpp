// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2024 David Zero <zero-one@zer0-one.net>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "wasm/instructions.h"

namespace wasm::instructions {

// clangd (16) crashes if this is = default even though though it's allowed and
// clang has alledegly implemented it starting with Clang 14:
// https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p2085r0.html
// https://clang.llvm.org/cxx_status.html
bool Block::operator==(Block const &b) const {
    return b.type == type && b.instructions == instructions;
}

bool Loop::operator==(Loop const &l) const {
    return l.type == type && l.instructions == instructions;
}

} // namespace wasm::instructions
