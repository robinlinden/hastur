// SPDX-FileCopyrightText: 2024 David Zero <zero-one@zer0-one.net>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "wasm/serialize.h"
#include "wasm/instructions.h"

#include <cstddef>
#include <optional>
#include <string>
#include <variant>

namespace wasm::instructions {

void InstructionStringifyVisitor::apply_indent() {
    for (std::size_t i = 0; i < indent; i++) {
        out += "\t";
    }
}

void InstructionStringifyVisitor::operator()(Block const &t) {
    out += Block::kMnemonic;
    out += " ";
    out += to_string(t.type);
    out += " ";

    indent++;

    for (Instruction const &i : t.instructions) {
        out += "\n";
        apply_indent();

        std::string subinst = to_string(i, *this);

        out += subinst;
    }

    indent--;

    out += "\n";
    apply_indent();
    out += "end";
}

void InstructionStringifyVisitor::operator()(Loop const &t) {
    out += Loop::kMnemonic;
    out += " ";
    out += to_string(t.type);
    out += " ";

    indent++;

    for (Instruction const &i : t.instructions) {
        out += "\n";
        apply_indent();

        std::string subinst = to_string(i, *this);

        out += subinst;
    }

    indent--;

    out += "\n";
    apply_indent();
    out += "end";
}

void InstructionStringifyVisitor::operator()(BreakIf const &t) {
    out += BreakIf::kMnemonic;
    out += " ";
    out += std::to_string(t.label_idx);
}

void InstructionStringifyVisitor::operator()(Return const &) {
    out += Return::kMnemonic;
}

void InstructionStringifyVisitor::operator()(I32Const const &t) {
    out += I32Const::kMnemonic;
    out += " ";
    out += std::to_string(t.value);
}

void InstructionStringifyVisitor::operator()(I32EqualZero const &) {
    out += I32EqualZero::kMnemonic;
}

void InstructionStringifyVisitor::operator()(I32Equal const &) {
    out += I32Equal::kMnemonic;
}

void InstructionStringifyVisitor::operator()(I32NotEqual const &) {
    out += I32NotEqual::kMnemonic;
}

void InstructionStringifyVisitor::operator()(I32LessThanSigned const &) {
    out += I32LessThanSigned::kMnemonic;
}

void InstructionStringifyVisitor::operator()(I32LessThanUnsigned const &) {
    out += I32LessThanUnsigned::kMnemonic;
}

void InstructionStringifyVisitor::operator()(I32GreaterThanSigned const &) {
    out += I32GreaterThanSigned::kMnemonic;
}

void InstructionStringifyVisitor::operator()(I32GreaterThanUnsigned const &) {
    out += I32GreaterThanUnsigned::kMnemonic;
}

void InstructionStringifyVisitor::operator()(I32LessThanEqualSigned const &) {
    out += I32LessThanEqualSigned::kMnemonic;
}

void InstructionStringifyVisitor::operator()(I32LessThanEqualUnsigned const &) {
    out += I32LessThanEqualUnsigned::kMnemonic;
}

void InstructionStringifyVisitor::operator()(I32GreaterThanEqualSigned const &) {
    out += I32GreaterThanEqualSigned::kMnemonic;
}

void InstructionStringifyVisitor::operator()(I32GreaterThanEqualUnsigned const &) {
    out += I32GreaterThanEqualUnsigned::kMnemonic;
}

void InstructionStringifyVisitor::operator()(I32CountLeadingZeros const &) {
    out += I32CountLeadingZeros::kMnemonic;
}

void InstructionStringifyVisitor::operator()(I32CountTrailingZeros const &) {
    out += I32CountTrailingZeros::kMnemonic;
}

void InstructionStringifyVisitor::operator()(I32PopulationCount const &) {
    out += I32PopulationCount::kMnemonic;
}

void InstructionStringifyVisitor::operator()(I32Add const &) {
    out += I32Add::kMnemonic;
}

void InstructionStringifyVisitor::operator()(I32Subtract const &) {
    out += I32Subtract::kMnemonic;
}

void InstructionStringifyVisitor::operator()(I32Multiply const &) {
    out += I32Multiply::kMnemonic;
}

void InstructionStringifyVisitor::operator()(I32DivideSigned const &) {
    out += I32DivideSigned::kMnemonic;
}

void InstructionStringifyVisitor::operator()(I32DivideUnsigned const &) {
    out += I32DivideUnsigned::kMnemonic;
}

void InstructionStringifyVisitor::operator()(I32RemainderSigned const &) {
    out += I32RemainderSigned::kMnemonic;
}

void InstructionStringifyVisitor::operator()(I32RemainderUnsigned const &) {
    out += I32RemainderUnsigned::kMnemonic;
}

void InstructionStringifyVisitor::operator()(I32And const &) {
    out += I32And::kMnemonic;
}

void InstructionStringifyVisitor::operator()(I32Or const &) {
    out += I32Or::kMnemonic;
}

void InstructionStringifyVisitor::operator()(I32ExclusiveOr const &) {
    out += I32ExclusiveOr::kMnemonic;
}

void InstructionStringifyVisitor::operator()(I32ShiftLeft const &) {
    out += I32ShiftLeft::kMnemonic;
}

void InstructionStringifyVisitor::operator()(I32ShiftRightSigned const &) {
    out += I32ShiftRightSigned::kMnemonic;
}

void InstructionStringifyVisitor::operator()(I32ShiftRightUnsigned const &) {
    out += I32ShiftRightUnsigned::kMnemonic;
}

void InstructionStringifyVisitor::operator()(I32RotateLeft const &) {
    out += I32RotateLeft::kMnemonic;
}

void InstructionStringifyVisitor::operator()(I32RotateRight const &) {
    out += I32RotateRight::kMnemonic;
}

void InstructionStringifyVisitor::operator()(I32WrapI64 const &) {
    out += I32WrapI64::kMnemonic;
}

void InstructionStringifyVisitor::operator()(I32TruncateF32Signed const &) {
    out += I32TruncateF32Signed::kMnemonic;
}

void InstructionStringifyVisitor::operator()(I32TruncateF32Unsigned const &) {
    out += I32TruncateF32Unsigned::kMnemonic;
}

void InstructionStringifyVisitor::operator()(I32TruncateF64Signed const &) {
    out += I32TruncateF64Signed::kMnemonic;
}

void InstructionStringifyVisitor::operator()(I32TruncateF64Unsigned const &) {
    out += I32TruncateF64Unsigned::kMnemonic;
}

void InstructionStringifyVisitor::operator()(I32ReinterpretF32 const &) {
    out += I32ReinterpretF32::kMnemonic;
}

void InstructionStringifyVisitor::operator()(I32Extend8Signed const &) {
    out += I32Extend8Signed::kMnemonic;
}

void InstructionStringifyVisitor::operator()(I32Extend16Signed const &) {
    out += I32Extend16Signed::kMnemonic;
}

void InstructionStringifyVisitor::operator()(LocalGet const &t) {
    out += LocalGet::kMnemonic;
    out += " ";
    out += std::to_string(t.idx);
}

void InstructionStringifyVisitor::operator()(LocalSet const &t) {
    out += LocalSet::kMnemonic;
    out += " ";
    out += std::to_string(t.idx);
}

void InstructionStringifyVisitor::operator()(LocalTee const &t) {
    out += LocalTee::kMnemonic;
    out += " ";
    out += std::to_string(t.idx);
}

void InstructionStringifyVisitor::operator()(I32Load const &t) {
    out += I32Load::kMnemonic;

    std::string memarg = to_string(t.arg, 32);

    if (!memarg.empty()) {
        out += " ";
        out += memarg;
    }
}

std::string to_string(Instruction const &inst, std::optional<InstructionStringifyVisitor> visitor) {
    if (visitor.has_value()) {
        visitor.value().out.clear();

        std::visit(*visitor, inst);

        return visitor.value().out;
    }

    InstructionStringifyVisitor v;

    std::visit(v, inst);

    return v.out;
}

} // namespace wasm::instructions
