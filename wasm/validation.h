// SPDX-FileCopyrightText: 2024 David Zero <zero-one@zer0-one.net>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef WASM_VALIDATION_H_
#define WASM_VALIDATION_H_

#include "wasm.h"
#include "wasm/instructions.h"
#include "wasm/types.h"

#include <cstdint>
#include <variant>
#include <vector>

namespace wasm::validation {

// https://webassembly.github.io/spec/core/valid/types.html#limits
constexpr bool is_valid(Limits const &l, std::uint64_t k) {
    if (l.min > k) {
        return false;
    }

    if (l.max.has_value()) {
        if (l.max > k || l.max < l.min) {
            return false;
        }
    }

    return true;
}

// https://webassembly.github.io/spec/core/valid/types.html#block-types
constexpr bool is_valid(wasm::instructions::BlockType &bt, Module const &m) {
    if (!m.type_section.has_value()) {
        return false;
    }

    if (auto *t = std::get_if<TypeIdx>(&bt.value)) {
        if (m.type_section->types.size() <= *t) {
            // Index outside bounds of defined types
            return false;
        }
    }

    return true;
}

// https://webassembly.github.io/spec/core/valid/types.html#table-types
constexpr bool is_valid(TableType const &t) {
    return is_valid(t.limits, (1ULL << 32) - 1);
}

// https://webassembly.github.io/spec/core/valid/types.html#memory-types
constexpr bool is_valid(MemType const &mt) {
    return is_valid(mt, 1ULL << 16);
}

// https://webassembly.github.io/spec/core/valid/types.html#match-limits
// https://webassembly.github.io/spec/core/valid/types.html#memories
constexpr bool is_match(Limits const &l1, Limits const &l2) {
    if (l1.min >= l2.min) {
        if (!l2.max.has_value()) {
            return true;
        }

        if (l1.max.has_value() && l1.max <= l2.max) {
            return true;
        }
    }

    return false;
}

// https://webassembly.github.io/spec/core/valid/types.html#functions
constexpr bool is_match(FunctionType const &f1, FunctionType const &f2) {
    return f1 == f2;
}

// https://webassembly.github.io/spec/core/valid/types.html#tables
constexpr bool is_match(TableType const &t1, TableType const &t2) {
    return is_match(t1.limits, t2.limits) && t1.element_type == t2.element_type;
}

// https://webassembly.github.io/spec/core/valid/types.html#globals
constexpr bool is_match(GlobalType const &g1, GlobalType const &g2) {
    return g1 == g2;
}

bool is_valid(std::vector<wasm::instructions::Instruction> const &inst);

} // namespace wasm::validation

#endif
