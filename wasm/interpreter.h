// SPDX-FileCopyrightText: 2023-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef WASM_INTERPRETER_H_
#define WASM_INTERPRETER_H_

#include "wasm/instructions.h"

#include <tl/expected.hpp>

#include <bit>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <optional>
#include <span>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

namespace wasm {
namespace detail {

template<typename T>
struct InterpreterInfo {};

template<>
struct InterpreterInfo<instructions::I32LessThanSigned> {
    using Operation = std::less<std::int32_t>;
};

template<>
struct InterpreterInfo<instructions::I32GreaterThanSigned> {
    using Operation = std::greater<std::int32_t>;
};

template<>
struct InterpreterInfo<instructions::I32LessThanEqualSigned> {
    using Operation = std::less_equal<std::int32_t>;
};

template<>
struct InterpreterInfo<instructions::I32GreaterThanEqualSigned> {
    using Operation = std::greater_equal<std::int32_t>;
};

template<>
struct InterpreterInfo<instructions::I32Add> {
    using Operation = std::plus<std::int32_t>;
};

template<>
struct InterpreterInfo<instructions::I32Subtract> {
    using Operation = std::minus<std::int32_t>;
};

template<>
struct InterpreterInfo<instructions::I32And> {
    using Operation = std::bit_and<std::int32_t>;
};

template<>
struct InterpreterInfo<instructions::I32Or> {
    using Operation = std::bit_or<std::int32_t>;
};

template<>
struct InterpreterInfo<instructions::I32ExclusiveOr> {
    using Operation = std::bit_xor<std::int32_t>;
};

template<>
struct InterpreterInfo<instructions::I64Equal> {
    using Operation = std::equal_to<std::int64_t>;
};

template<>
struct InterpreterInfo<instructions::I64LessThanSigned> {
    using Operation = std::less<std::int64_t>;
};

template<>
struct InterpreterInfo<instructions::I64Add> {
    using Operation = std::plus<std::int64_t>;
};

template<>
struct InterpreterInfo<instructions::I64Subtract> {
    using Operation = std::minus<std::int64_t>;
};

template<>
struct InterpreterInfo<instructions::I64Multiply> {
    using Operation = std::multiplies<std::int64_t>;
};

} // namespace detail

enum class Trap : std::uint8_t {
    MemoryAccessOutOfBounds,
    UnhandledInstruction,
};

class Interpreter {
public:
    using Value = std::variant<std::int32_t, std::int64_t, float>;

    struct Function {
        std::vector<instructions::Instruction> body;
        std::uint32_t local_count{};
        [[nodiscard]] bool operator==(Function const &) const = default;
    };

    std::vector<Value> stack;
    std::vector<Value> locals;
    std::vector<Value> globals;
    std::vector<std::uint8_t> memory;
    std::vector<Function> functions;
    bool returning{false};
    [[nodiscard]] constexpr bool operator==(Interpreter const &) const = default;

    tl::expected<std::optional<Value>, Trap> run(std::span<instructions::Instruction const>);

    template<typename T>
    tl::expected<void, Trap> interpret(T const &) {
        std::cerr << "Unhandled instruction: " << T::kMnemonic << '\n';
        return tl::unexpected{Trap::UnhandledInstruction};
    }

    // https://webassembly.github.io/spec/core/exec/instructions.html#blocks
    // In the flat execution model (no nested blocks), end is the function body terminator.
    // run() already returns stack.back() after the loop, so this is a no-op.
    tl::expected<void, Trap> interpret(instructions::End const &) { return {}; }

    // https://webassembly.github.io/spec/core/exec/instructions.html#returning-from-a-function
    // (frame_n { f }  val'*  val^n  return  instr*)  →  val^n
    // Sets the returning flag so run() stops executing further instructions.
    tl::expected<void, Trap> interpret(instructions::Return const &) {
        returning = true;
        return {};
    }

    // https://webassembly.github.io/spec/core/exec/instructions.html#function-calls
    // Invoke function[idx] in its own isolated frame; push its result (if any) onto our stack.
    tl::expected<void, Trap> interpret(instructions::Call const &call) {
        assert(call.function_idx < functions.size());
        auto const &fn = functions[call.function_idx];

        auto saved_locals = std::exchange(locals, std::vector<Value>(fn.local_count));
        auto saved_returning = std::exchange(returning, false);

        auto result = run(fn.body);

        // Restore caller frame.
        locals = std::move(saved_locals);
        returning = saved_returning;

        if (!result) {
            return tl::unexpected{result.error()};
        }
        auto &retval = result.value();
        if (retval.has_value()) {
            stack.push_back(*retval);
        }
        return {};
    }

    // https://webassembly.github.io/spec/core/exec/instructions.html#numeric-instructions
    // t.const c
    tl::expected<void, Trap> interpret(instructions::I32Const const &v) {
        stack.emplace_back(v.value);
        return {};
    }

    tl::expected<void, Trap> interpret(instructions::I64Const const &v) {
        stack.emplace_back(v.value);
        return {};
    }

    tl::expected<void, Trap> interpret(instructions::F32Const const &v) {
        stack.emplace_back(v.value);
        return {};
    }

    template<typename T>
    requires(T::kNumericType == instructions::NumericType::Relop)
    tl::expected<void, Trap> interpret(T const &) {
        // TODO(robinlinden): trap.
        assert(stack.size() >= 2);
        auto rhs = std::get<typename T::NumType>(stack.back());
        stack.pop_back();
        auto lhs = std::get<typename T::NumType>(stack.back());
        stack.pop_back();
        stack.emplace_back(typename detail::InterpreterInfo<T>::Operation{}(lhs, rhs) ? 1 : 0);
        return {};
    }

    template<typename T>
    requires(T::kNumericType == instructions::NumericType::Binop)
    tl::expected<void, Trap> interpret(T const &) {
        // TODO(robinlinden): trap.
        assert(stack.size() >= 2);
        auto rhs = std::get<typename T::NumType>(stack.back());
        stack.pop_back();
        auto lhs = std::get<typename T::NumType>(stack.back());
        stack.pop_back();
        stack.emplace_back(typename detail::InterpreterInfo<T>::Operation{}(lhs, rhs));
        return {};
    }

    // https://webassembly.github.io/spec/core/exec/instructions.html#variable-instructions
    tl::expected<void, Trap> interpret(instructions::LocalGet const &v) {
        stack.push_back(locals.at(v.idx));
        return {};
    }

    tl::expected<void, Trap> interpret(instructions::LocalSet const &v) {
        assert(!stack.empty());
        locals.at(v.idx) = stack.back();
        stack.pop_back();
        return {};
    }

    tl::expected<void, Trap> interpret(instructions::LocalTee const &v) {
        assert(!stack.empty());
        locals.at(v.idx) = stack.back();
        return {};
    }

    tl::expected<void, Trap> interpret(instructions::GlobalGet const &v) {
        stack.push_back(globals.at(v.global_idx));
        return {};
    }

    tl::expected<void, Trap> interpret(instructions::GlobalSet const &v) {
        assert(!stack.empty());
        globals.at(v.global_idx) = stack.back();
        stack.pop_back();
        return {};
    }

    // https://webassembly.github.io/spec/core/exec/instructions.html#memory-instructions
    tl::expected<void, Trap> interpret(instructions::I32Load const &v) {
        assert(!stack.empty());
        auto [align, offset] = v.arg;
        std::ignore = align;

        auto i = std::get<std::int32_t>(stack.back());
        stack.pop_back();
        auto ea = i + offset;

        if (ea + sizeof(std::int32_t) > memory.size()) {
            return tl::unexpected{Trap::MemoryAccessOutOfBounds};
        }

        std::int32_t value{};
        std::memcpy(&value, memory.data() + ea, sizeof(value));

        static_assert((std::endian::native == std::endian::big) || (std::endian::native == std::endian::little),
                "Mixed endian is unsupported right now");
        if constexpr (std::endian::native != std::endian::little) {
            value = std::byteswap(value);
        }

        stack.emplace_back(value);
        return {};
    }

    tl::expected<void, Trap> interpret(instructions::I32Store const &v) {
        assert(stack.size() >= 2);
        auto [align, offset] = v.arg;
        std::ignore = align;

        auto to_store = std::get<std::int32_t>(stack.back());
        stack.pop_back();
        auto i = std::get<std::int32_t>(stack.back());
        stack.pop_back();
        auto ea = i + offset;

        if (ea + sizeof(std::int32_t) > memory.size()) {
            return tl::unexpected{Trap::MemoryAccessOutOfBounds};
        }

        static_assert((std::endian::native == std::endian::big) || (std::endian::native == std::endian::little),
                "Mixed endian is unsupported right now");
        if constexpr (std::endian::native != std::endian::little) {
            to_store = std::byteswap(to_store);
        }

        std::memcpy(memory.data() + ea, &to_store, sizeof(to_store));
        return {};
    }
};

inline tl::expected<std::optional<Interpreter::Value>, Trap> Interpreter::run(
        std::span<instructions::Instruction const> insns) {
    returning = false;
    for (auto const &insn : insns) {
        auto res = std::visit(
                [this](auto const &i) -> tl::expected<void, Trap> {
                    return this->interpret(i); //
                },
                insn);
        if (!res) {
            return tl::unexpected{res.error()};
        }
        if (returning) {
            break;
        }
    }
    returning = false;

    return stack.empty() ? std::nullopt : std::optional{stack.back()};
}

} // namespace wasm

#endif
