// SPDX-FileCopyrightText: 2024 David Zero <zero-one@zer0-one.net>
// SPDX-FileCopyrightText: 2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "wasm/serialize.h"

#include "wasm/instructions.h"

#include <cstddef>
#include <optional>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

namespace wasm::instructions {
namespace {

struct InstructionStringifyVisitor {
    std::stringstream out;
    std::size_t indent = 0;

    void apply_indent();

    void operator()(Block const &t);
    void operator()(Loop const &t);
    void operator()(BreakIf const &t);
    void operator()(Return const &);
    void operator()(I32Const const &t);
    void operator()(LocalGet const &t);
    void operator()(LocalSet const &t);
    void operator()(LocalTee const &t);
    void operator()(I32Load const &t);

    template<typename T>
    requires std::is_empty_v<T>
    void operator()(T const &);
};

void InstructionStringifyVisitor::apply_indent() {
    for (std::size_t i = 0; i < indent; i++) {
        out << "\t";
    }
}

void InstructionStringifyVisitor::operator()(Block const &t) {
    out << Block::kMnemonic << " " << to_string(t.type) << " ";

    indent++;

    for (Instruction const &i : t.instructions) {
        out << "\n";
        apply_indent();
        std::visit(*this, i);
    }

    indent--;

    out << "\n";
    apply_indent();
    out << "end";
}

void InstructionStringifyVisitor::operator()(Loop const &t) {
    out << Loop::kMnemonic << " " << to_string(t.type) << " ";

    indent++;

    for (Instruction const &i : t.instructions) {
        out << "\n";
        apply_indent();
        std::visit(*this, i);
    }

    indent--;

    out << "\n";
    apply_indent();
    out << "end";
}

void InstructionStringifyVisitor::operator()(BreakIf const &t) {
    out << BreakIf::kMnemonic << " " << std::to_string(t.label_idx);
}

void InstructionStringifyVisitor::operator()(Return const &) {
    out << Return::kMnemonic;
}

void InstructionStringifyVisitor::operator()(I32Const const &t) {
    out << I32Const::kMnemonic << " " << std::to_string(t.value);
}

template<typename T>
requires std::is_empty_v<T>
void InstructionStringifyVisitor::operator()(T const &) {
    out << T::kMnemonic;
}

void InstructionStringifyVisitor::operator()(LocalGet const &t) {
    out << LocalGet::kMnemonic << " " << std::to_string(t.idx);
}

void InstructionStringifyVisitor::operator()(LocalSet const &t) {
    out << LocalSet::kMnemonic << " " << std::to_string(t.idx);
}

void InstructionStringifyVisitor::operator()(LocalTee const &t) {
    out << LocalTee::kMnemonic << " " << std::to_string(t.idx);
}

void InstructionStringifyVisitor::operator()(I32Load const &t) {
    out << I32Load::kMnemonic;

    std::string memarg = to_string(t.arg, 32);

    if (!memarg.empty()) {
        out << " " << memarg;
    }
}

} // namespace

std::string to_string(Instruction const &inst) {
    InstructionStringifyVisitor v;
    std::visit(v, inst);
    return std::move(v.out).str();
}

} // namespace wasm::instructions
