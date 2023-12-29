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

void InstructionStringifyVisitor::operator()(I32LessThanSigned const &) {
    out += I32LessThanSigned::kMnemonic;
}

void InstructionStringifyVisitor::operator()(I32Add const &) {
    out += I32Add::kMnemonic;
}

void InstructionStringifyVisitor::operator()(I32Sub const &) {
    out += I32Sub::kMnemonic;
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
