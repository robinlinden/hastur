// SPDX-FileCopyrightText: 2024 David Zero <zero-one@zer0-one.net>
// SPDX-FileCopyrightText: 2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "wasm/validation.h"

#include "wasm/instructions.h"
#include "wasm/types.h"
#include "wasm/wasm.h"

#include "etest/etest2.h"

#include <tl/expected.hpp>

#include <vector>

int main() {
    etest::Suite s{"wasm::validation"};

    using namespace wasm;
    using namespace wasm::instructions;
    using namespace wasm::validation;

    Module m{};
    m.function_section = FunctionSection{.type_indices = {0}};
    m.type_section = TypeSection{.types = {FunctionType{.parameters = {}, .results = {ValueType::Int32}}}};
    m.code_section = CodeSection{.entries = {CodeEntry{}}};

    s.add_test("Function: empty sequence", [=](etest::IActions &a) { a.expect(validate(m).has_value()); });

    s.add_test("Function: valid trivial sequence", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {I32Const{42}, I32Const{42}, I32Add{}, I32CountLeadingZeros{}};

        a.expect(validate(m).has_value());
    });

    s.add_test("Function: invalid trivial sequence", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {I32Const{42}, I32Add{}};

        a.expect_eq(validate(m), tl::unexpected{ValidationError::ValueStackUnderflow});
    });

    s.add_test("Function: block with valid body", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {
                Block{.type = {ValueType::Int32}, .instructions = {I32Const{42}, I32Const{42}, I32Add{}}}};

        a.expect(validate(m).has_value());
    });

    s.add_test("Function: loop with valid body", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {
                Loop{.type = {ValueType::Int32}, .instructions = {I32Const{42}, I32Const{42}, I32Add{}}}};

        a.expect(validate(m).has_value());
    });

    s.add_test("Function: block with invalid body", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {Block{.type = {ValueType::Int32}, .instructions = {I32Const{42}, I32Add{}}}};

        a.expect_eq(validate(m), tl::unexpected{ValidationError::ValueStackUnderflow});
    });

    s.add_test("Function: block returning with unclean stack", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {Block{
                .type = {ValueType::Int32}, .instructions = {I32Const{42}, I32Const{42}, I32Const{42}, I32Add{}}}};

        a.expect_eq(validate(m), tl::unexpected{ValidationError::ValueStackHeightMismatch});
    });

    s.add_test("Function: block with valid body and invalid return value", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {
                Block{.type = {ValueType::Int64}, .instructions = {I32Const{42}, I32Const{42}, I32Add{}}}};

        a.expect_eq(validate(m), tl::unexpected{ValidationError::ValueStackUnexpected});
    });

    s.add_test("Function: block ending with branch", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {
                Block{.type = {ValueType::Int32}, .instructions = {I32Const{42}, Branch{.label_idx = 0}}}};

        a.expect(validate(m).has_value());
    });

    s.add_test("Function: loop with conditional branch", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {
                Loop{.type = {BlockType::Empty{}}, .instructions = {I32Const{1}, BranchIf{.label_idx = 0}}},
                I32Const{1}};

        a.expect(validate(m).has_value());
    });

    s.add_test("Function: loop with conditional branch, invalid label", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {
                Loop{.type = {BlockType::Empty{}}, .instructions = {I32Const{1}, BranchIf{.label_idx = 4}}}};

        a.expect_eq(validate(m), tl::unexpected{ValidationError::LabelInvalid});
    });

    s.add_test("Function: block with branch, dead code", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {Block{.type = {ValueType::Int32},
                .instructions = {I32Const{42}, I32Const{42}, Branch{.label_idx = 0}, I32Add{}}}};

        a.expect(validate(m).has_value());
    });

    s.add_test("Function: block with branch, incorrect return value", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {
                Block{.type = {ValueType::Int64}, .instructions = {I32Const{42}, Branch{.label_idx = 0}}}};

        a.expect_eq(validate(m), tl::unexpected{ValidationError::ValueStackUnexpected});
    });

    s.add_test("Function: block with branch, invalid label", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {Block{.type = {ValueType::Int32}, .instructions = {Branch{.label_idx = 4}}}};

        a.expect_eq(validate(m), tl::unexpected{ValidationError::LabelInvalid});
    });

    s.add_test("Function: block with type use, missing type section", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {Block{.type = {TypeIdx{1}}}};

        a.expect_eq(validate(m), tl::unexpected{ValidationError::BlockTypeInvalid});
    });

    s.add_test("Function: getting undefined local", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {Block{.type = {ValueType::Int32}, .instructions = {LocalGet{0}}}};

        a.expect_eq(validate(m), tl::unexpected{ValidationError::LocalUndefined});
    });

    s.add_test("Function: valid return", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {Block{.type = {ValueType::Int32}, .instructions = {I32Const{42}, Return{}}}};

        a.expect(validate(m).has_value());
    });

    s.add_test("Function: invalid return, implicit", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {
                Loop{.type = {BlockType::Empty{}}, .instructions = {I32Const{1}, BranchIf{.label_idx = 0}}}};

        a.expect_eq(validate(m), tl::unexpected{ValidationError::ValueStackUnderflow});
    });

    s.add_test("Function: invalid return, explicit", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {
                Loop{.type = {BlockType::Empty{}}, .instructions = {I32Const{1}, BranchIf{.label_idx = 0}}}, Return{}};

        a.expect_eq(validate(m), tl::unexpected{ValidationError::ValueStackUnderflow});
    });

    s.add_test("Function: load, no memory section defined", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {I32Const{0}, I32Load{}};

        a.expect_eq(validate(m), tl::unexpected{ValidationError::MemorySectionUndefined});
    });

    s.add_test("Function: load, memory empty", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {I32Const{0}, I32Load{}};
        m.memory_section = MemorySection{};

        a.expect_eq(validate(m), tl::unexpected{ValidationError::MemoryEmpty});
    });

    s.add_test("Function: load, bad alignment", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {I32Const{0}, I32Load{.arg = {.align = 5}}};
        m.memory_section = MemorySection{.memories = {MemType{.min = 42}}};

        a.expect_eq(validate(m), tl::unexpected{ValidationError::MemoryBadAlignment});
    });

    s.add_test("Function: load, missing arg", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {I32Load{}};
        m.memory_section = MemorySection{.memories = {MemType{.min = 42}}};

        a.expect_eq(validate(m), tl::unexpected{ValidationError::ValueStackUnderflow});
    });

    s.add_test("Function: valid load", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {I32Const{0}, I32Load{}};
        m.memory_section = MemorySection{.memories = {MemType{.min = 42}}};

        a.expect(validate(m).has_value());
    });

    s.add_test("Function: localset & localget, valid", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {I32Const{42}, LocalSet{.idx = 0}, LocalGet{.idx = 0}};
        m.code_section->entries[0].locals = {{.count = 1, .type = ValueType::Int32}};

        a.expect(validate(m).has_value());
    });

    s.add_test("Function: localset & localget, missing arg", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {LocalSet{.idx = 0}, LocalGet{.idx = 0}};
        m.code_section->entries[0].locals = {{.count = 1, .type = ValueType::Int32}};

        a.expect_eq(validate(m), tl::unexpected{ValidationError::ValueStackUnderflow});
    });

    s.add_test("Function: localtee, valid", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {I32Const{42}, LocalTee{.idx = 0}};
        m.code_section->entries[0].locals = {{.count = 1, .type = ValueType::Int32}};

        a.expect(validate(m).has_value());
    });

    s.add_test("Function: localtee, missing arg", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {LocalTee{.idx = 0}};
        m.code_section->entries[0].locals = {{.count = 1, .type = ValueType::Int32}};

        a.expect_eq(validate(m), tl::unexpected{ValidationError::ValueStackUnderflow});
    });

    return s.run();
}
