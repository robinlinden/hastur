// SPDX-FileCopyrightText: 2023-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef WASM_INTERPRETER_H_
#define WASM_INTERPRETER_H_

#include "wasm/instructions.h"

#include <bit>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <optional>
#include <span>
#include <tuple>
#include <variant>
#include <vector>

namespace wasm {

class Interpreter {
public:
    using Value = std::variant<std::int32_t>;
    std::vector<Value> stack;
    std::vector<Value> locals;
    std::vector<std::uint8_t> memory;
    [[nodiscard]] constexpr bool operator==(Interpreter const &) const = default;

    std::optional<Value> run(std::span<instructions::Instruction const> const &insns) {
        for (auto const &insn : insns) {
            std::visit([this](auto const &i) { this->interpret(i); }, insn);
        }

        return stack.empty() ? std::nullopt : std::optional{stack.back()};
    }

    template<typename T>
    void interpret(T const &) {
        std::cerr << "Unhandled instruction: " << T::kMnemonic << '\n';
    }

    // https://webassembly.github.io/spec/core/exec/instructions.html#numeric-instructions
    // t.const c
    void interpret(instructions::I32Const const &v) { stack.emplace_back(v.value); }

    // t.binop
    void interpret(instructions::I32LessThanSigned const &) {
        // TODO(robinlinden): trap.
        assert(stack.size() >= 2);
        auto rhs = std::get<std::int32_t>(stack.back());
        stack.pop_back();
        auto lhs = std::get<std::int32_t>(stack.back());
        stack.pop_back();
        stack.emplace_back(lhs < rhs ? 1 : 0);
    }

    void interpret(instructions::I32Add const &) {
        // TODO(robinlinden): trap.
        assert(stack.size() >= 2);
        auto rhs = std::get<std::int32_t>(stack.back());
        stack.pop_back();
        auto lhs = std::get<std::int32_t>(stack.back());
        stack.pop_back();
        stack.emplace_back(lhs + rhs);
    }

    // https://webassembly.github.io/spec/core/exec/instructions.html#variable-instructions
    void interpret(instructions::LocalGet const &v) { stack.push_back(locals.at(v.idx)); }

    void interpret(instructions::LocalSet const &v) {
        assert(!stack.empty());
        locals.at(v.idx) = stack.back();
        stack.pop_back();
    }

    void interpret(instructions::LocalTee const &v) {
        assert(!stack.empty());
        locals.at(v.idx) = stack.back();
    }

    // https://webassembly.github.io/spec/core/exec/instructions.html#memory-instructions
    void interpret(instructions::I32Load const &v) {
        // TODO(robinlinden): trap.
        assert(!stack.empty());
        auto [align, offset] = v.arg;
        std::ignore = align;

        auto i = std::get<std::int32_t>(stack.back());
        stack.pop_back();
        auto ea = i + offset;

        assert(ea + sizeof(std::int32_t) <= memory.size());

        std::int32_t value{};
        std::memcpy(&value, memory.data() + ea, sizeof(value));

        static_assert((std::endian::native == std::endian::big) || (std::endian::native == std::endian::little),
                "Mixed endian is unsupported right now");
        if constexpr (std::endian::native != std::endian::little) {
            value = std::byteswap(value);
        }

        stack.emplace_back(value);
    }
};

} // namespace wasm

#endif
