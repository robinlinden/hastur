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
    void operator()(I32LessThanSigned const &);
    void operator()(I32Add const &);
    void operator()(I32Sub const &);
    void operator()(LocalGet const &t);
    void operator()(LocalSet const &t);
    void operator()(LocalTee const &t);
    void operator()(I32Load const &t);
};

std::string to_string(Instruction const &inst, std::optional<InstructionStringifyVisitor> = std::nullopt);

} // namespace wasm::instructions

#endif
