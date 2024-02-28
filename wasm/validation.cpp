// SPDX-FileCopyrightText: 2024 David Zero <zero-one@zer0-one.net>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "wasm/validation.h"
#include "wasm/instructions.h"
#include "wasm/types.h"

#include <optional>
#include <variant>
#include <vector>

namespace wasm::validation {

using namespace wasm;
using namespace wasm::instructions;

struct ControlFrame {
    Instruction i;
    std::vector<ValueType> start_stack;
    std::vector<ValueType> end_stack;
    unsigned int stack_height;
    bool unreachable;
};

// TODO(dzero): Add error information
namespace {

void push_val(std::vector<ValueType> &value_stack, ValueType const &val) {
    value_stack.emplace_back(val);
};

std::optional<ValueType> pop_val(std::vector<ValueType> &value_stack, std::vector<ControlFrame> &control_stack) {
    if (control_stack.empty()) {
        return std::nullopt;
    }

    if (value_stack.size() == control_stack.back().stack_height && control_stack.back().unreachable) {
        return ValueType::Unknown;
    }

    if (value_stack.size() == control_stack.back().stack_height) {
        return std::nullopt;
    }

    auto val = value_stack.back();

    value_stack.pop_back();

    return val;
};

std::optional<ValueType> pop_val_expect(
        std::vector<ValueType> &value_stack, std::vector<ControlFrame> &control_stack, ValueType const &expected) {
    auto actual = pop_val(value_stack, control_stack);

    if (actual != expected && actual != ValueType::Unknown && expected != ValueType::Unknown) {
        return std::nullopt;
    }

    return actual;
};

void push_vals(std::vector<ValueType> &value_stack, std::vector<ValueType> const &vals) {
    for (ValueType const &val : vals) {
        value_stack.emplace_back(val);
    }
};

std::optional<std::vector<ValueType>> pop_vals(std::vector<ValueType> &value_stack,
        std::vector<ControlFrame> &control_stack,
        std::vector<ValueType> const &vals) {
    std::vector<ValueType> popped;

    for (ValueType const &v : vals) {
        auto maybe_val = pop_val_expect(value_stack, control_stack, v);

        if (!maybe_val.has_value()) {
            return std::nullopt;
        }

        popped.emplace_back(*maybe_val);
    }

    return popped;
};

void push_ctrl(std::vector<ControlFrame> &control_stack,
        std::vector<ValueType> &value_stack,
        Instruction i,
        std::vector<ValueType> start,
        std::vector<ValueType> end) {
    control_stack.emplace_back(i, start, end, value_stack.size(), false);

    push_vals(value_stack, start);
}

std::optional<ControlFrame> pop_ctrl(std::vector<ControlFrame> &control_stack, std::vector<ValueType> &value_stack) {
    if (control_stack.empty()) {
        return std::nullopt;
    }

    auto frame = control_stack.back();

    pop_vals(value_stack, control_stack, frame.end_stack);

    if (value_stack.size() != frame.stack_height) {
        return std::nullopt;
    }

    control_stack.pop_back();

    return frame;
}

void unreachable(std::vector<ControlFrame> &control_stack, std::vector<ValueType> &value_stack) {
    value_stack.resize(control_stack.back().stack_height);

    control_stack.back().unreachable = true;
}

} // namespace

bool is_valid(std::vector<Instruction> const &inst) {
    // https://webassembly.github.io/spec/core/valid/instructions.html#empty-instruction-sequence-epsilon
    if (inst.empty()) {
        return true;
    }

    std::vector<ValueType> value_stack; // operand stack
    std::vector<ControlFrame> control_stack;

    // auto push_ctrl = [&control_stack](Instruction i, std::vector<ValueType> )

    for (auto const &i : inst) {
        if (std::holds_alternative<I32Add>(i) || std::holds_alternative<I32Subtract>(i)) {
            if (pop_vals(value_stack, control_stack, {ValueType::Int32, ValueType::Int32}) == std::nullopt) {
                return false;
            }

            push_val(value_stack, ValueType::Int32);
        }
    }

    // Once done testing, maybe implement visitor pattern below

    /*
        template<class... Ts>
        struct InstVisitorOverload : Ts... { using Ts::operator()...; };
    */

    /*
        for(auto i : inst){
            std::visit(InstVisitorOverload{
                [&](I32Add) {push_val(Int32)},
            }, i);
        }
    */
    return true;
}

} // namespace wasm::validation
