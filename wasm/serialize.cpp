// SPDX-FileCopyrightText: 2024 David Zero <zero-one@zer0-one.net>
// SPDX-FileCopyrightText: 2024-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "wasm/serialize.h"

#include "wasm/instructions.h"

#include <cstddef>
#include <optional>
#include <span>
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
    void operator()(Branch const &t);
    void operator()(BranchIf const &t);
    void operator()(Call const &t);
    void operator()(I32Const const &t);
    void operator()(LocalGet const &t);
    void operator()(LocalSet const &t);
    void operator()(LocalTee const &t);
    void operator()(GlobalGet const &t);
    void operator()(GlobalSet const &t);
    void operator()(I32Load const &t);
    void operator()(I32Store const &t);

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
    out << Block::kMnemonic << " " << to_string(t.type);
    indent++;
}

void InstructionStringifyVisitor::operator()(Loop const &t) {
    out << Loop::kMnemonic << " " << to_string(t.type);
    indent++;
}

void InstructionStringifyVisitor::operator()(Branch const &t) {
    out << Branch::kMnemonic << " " << std::to_string(t.label_idx);
}

void InstructionStringifyVisitor::operator()(BranchIf const &t) {
    out << BranchIf::kMnemonic << " " << std::to_string(t.label_idx);
}

void InstructionStringifyVisitor::operator()(Call const &t) {
    out << Call::kMnemonic << ' ' << std::to_string(t.function_idx);
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

void InstructionStringifyVisitor::operator()(GlobalGet const &t) {
    out << GlobalGet::kMnemonic << ' ' << std::to_string(t.global_idx);
}

void InstructionStringifyVisitor::operator()(GlobalSet const &t) {
    out << GlobalSet::kMnemonic << ' ' << std::to_string(t.global_idx);
}

void InstructionStringifyVisitor::operator()(I32Load const &t) {
    out << I32Load::kMnemonic;

    std::string memarg = to_string(t.arg, 32);

    if (!memarg.empty()) {
        out << " " << memarg;
    }
}

void InstructionStringifyVisitor::operator()(I32Store const &t) {
    out << I32Store::kMnemonic;

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

// TODO(robinlinden): Nicer handling of indentation. End should dedent and
// block/loop should indent.
std::string to_string(std::span<Instruction const> insns) {
    InstructionStringifyVisitor v;
    for (std::size_t i = 0; i < insns.size(); ++i) {
        auto const &insn = insns[i];
        if (std::holds_alternative<End>(insn) && v.indent > 0) {
            v.indent--;
        }

        v.apply_indent();
        std::visit(v, insn);

        if (i != insns.size() - 1) {
            v.out << '\n';
        }
    }
    return std::move(v.out).str();
}

} // namespace wasm::instructions
