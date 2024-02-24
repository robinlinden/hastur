// SPDX-FileCopyrightText: 2024 David Zero <zero-one@zer0-one.net>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef WASM_SERIALIZE_H
#define WASM_SERIALIZE_H

#include "wasm/instructions.h"
#include "wasm/wasm.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace wasm {

constexpr std::string_view to_string(ValueType const &vt) {
    switch (vt.kind) {
        case ValueType::Kind::Int32:
            return "i32";
        case ValueType::Kind::Int64:
            return "i64";
        case ValueType::Kind::Float32:
            return "f32";
        case ValueType::Kind::Float64:
            return "f64";
        case ValueType::Kind::Vector128:
            return "v128";
        case ValueType::Kind::FunctionReference:
            return "funcref";
        case ValueType::Kind::ExternReference:
            return "externref";
    }

    return "UNHANDLED_VALUE_TYPE";
}

} // namespace wasm

namespace wasm::instructions {

constexpr std::string to_string(BlockType const &bt) {
    std::string out;

    if (ValueType const *v = std::get_if<ValueType>(&bt.value)) {
        out += "(result ";
        out += wasm::to_string(*v);
        out += ")";
    } else if (TypeIdx const *t = std::get_if<TypeIdx>(&bt.value)) {
        out += "(type ";
        out += std::to_string(*t);
        out += ")";
    } else {
        assert(std::holds_alternative<BlockType::Empty>(bt.value));
    }

    return out;
}

constexpr std::string to_string(MemArg const &ma, std::optional<std::uint32_t> natural_alignment = std::nullopt) {
    std::string out;

    // If offset == 0, omit offset
    if (ma.offset != 0) {
        out += "offset=";
        out += std::to_string(ma.offset);
    }

    // Natural alignment, omit "align=" phrase
    if (natural_alignment == ma.align) {
        return out;
    }

    if (ma.offset != 0) {
        out += " ";
    }

    out += "align=";
    out += std::to_string(ma.align);

    return out;
}

struct InstructionStringifyVisitor {
    std::string out;
    std::size_t indent = 0;

    void apply_indent();

    void operator()(Block const &t);
    void operator()(Loop const &t);
    void operator()(BreakIf const &t);
    void operator()(Return const &);
    void operator()(I32Const const &t);
    void operator()(I32EqualZero const &);
    void operator()(I32Equal const &);
    void operator()(I32NotEqual const &);
    void operator()(I32LessThanSigned const &);
    void operator()(I32LessThanUnsigned const &);
    void operator()(I32GreaterThanSigned const &);
    void operator()(I32GreaterThanUnsigned const &);
    void operator()(I32LessThanEqualSigned const &);
    void operator()(I32LessThanEqualUnsigned const &);
    void operator()(I32GreaterThanEqualSigned const &);
    void operator()(I32GreaterThanEqualUnsigned const &);
    void operator()(I32CountLeadingZeros const &);
    void operator()(I32CountTrailingZeros const &);
    void operator()(I32PopulationCount const &);
    void operator()(I32Add const &);
    void operator()(I32Subtract const &);
    void operator()(I32Multiply const &);
    void operator()(I32DivideSigned const &);
    void operator()(I32DivideUnsigned const &);
    void operator()(I32RemainderSigned const &);
    void operator()(I32RemainderUnsigned const &);
    void operator()(I32And const &);
    void operator()(I32Or const &);
    void operator()(I32ExclusiveOr const &);
    void operator()(I32ShiftLeft const &);
    void operator()(I32ShiftRightSigned const &);
    void operator()(I32ShiftRightUnsigned const &);
    void operator()(I32RotateLeft const &);
    void operator()(I32RotateRight const &);
    void operator()(I32WrapI64 const &);
    void operator()(I32TruncateF32Signed const &);
    void operator()(I32TruncateF32Unsigned const &);
    void operator()(I32TruncateF64Signed const &);
    void operator()(I32TruncateF64Unsigned const &);
    void operator()(I32ReinterpretF32 const &);
    void operator()(I32Extend8Signed const &);
    void operator()(I32Extend16Signed const &);
    void operator()(LocalGet const &t);
    void operator()(LocalSet const &t);
    void operator()(LocalTee const &t);
    void operator()(I32Load const &t);
};

std::string to_string(Instruction const &inst, std::optional<InstructionStringifyVisitor> = std::nullopt);

} // namespace wasm::instructions

#endif
