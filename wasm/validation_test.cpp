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

#include <cstddef>
#include <string>
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
                Block{.type = {ValueType::Int32}}, I32Const{42}, I32Const{42}, I32Add{}, End{}};

        a.expect(validate(m).has_value());
    });

    s.add_test("Function: loop with valid body", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {
                Loop{.type = {ValueType::Int32}}, I32Const{42}, I32Const{42}, I32Add{}, End{}};

        a.expect(validate(m).has_value());
    });

    s.add_test("Function: block with invalid body", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {Block{.type = {ValueType::Int32}}, I32Const{42}, I32Add{}, End{}};

        a.expect_eq(validate(m), tl::unexpected{ValidationError::ValueStackUnderflow});
    });

    s.add_test("Function: block returning with unclean stack", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {
                Block{.type = {ValueType::Int32}}, I32Const{42}, I32Const{42}, I32Const{42}, I32Add{}, End{}};

        a.expect_eq(validate(m), tl::unexpected{ValidationError::ValueStackHeightMismatch});
    });

    s.add_test("Function: block with valid body and invalid return value", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {
                Block{.type = {ValueType::Int64}}, I32Const{42}, I32Const{42}, I32Add{}, End{}};

        a.expect_eq(validate(m), tl::unexpected{ValidationError::ValueStackUnexpected});
    });

    s.add_test("Function: block ending with branch", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {
                Block{.type = {ValueType::Int32}}, I32Const{42}, Branch{.label_idx = 0}, End{}};

        a.expect(validate(m).has_value());
    });

    s.add_test("Function: loop with conditional branch", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {
                Loop{.type = {BlockType::Empty{}}}, I32Const{1}, BranchIf{.label_idx = 0}, End{}, I32Const{1}, End{}};

        a.expect(validate(m).has_value());
    });

    s.add_test("Function: loop with conditional branch, invalid label", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {
                Loop{.type = {BlockType::Empty{}}}, I32Const{1}, BranchIf{.label_idx = 4}, End{}};

        a.expect_eq(validate(m), tl::unexpected{ValidationError::LabelInvalid});
    });

    s.add_test("Function: block with branch, dead code", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {
                Block{.type = {ValueType::Int32}}, I32Const{42}, I32Const{42}, Branch{.label_idx = 0}, I32Add{}, End{}};

        a.expect(validate(m).has_value());
    });

    s.add_test("Function: block with branch, incorrect return value", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {
                Block{.type = {ValueType::Int64}}, I32Const{42}, Branch{.label_idx = 0}, End{}};

        a.expect_eq(validate(m), tl::unexpected{ValidationError::ValueStackUnexpected});
    });

    s.add_test("Function: block with branch, invalid label", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {Block{.type = {ValueType::Int32}}, Branch{.label_idx = 4}, End{}};

        a.expect_eq(validate(m), tl::unexpected{ValidationError::LabelInvalid});
    });

    s.add_test("Function: block with type use, missing type section", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {Block{.type = {TypeIdx{1}}}};

        a.expect_eq(validate(m), tl::unexpected{ValidationError::BlockTypeInvalid});
    });

    s.add_test("Function: getting undefined local", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {Block{.type = {ValueType::Int32}}, LocalGet{0}, End{}};

        a.expect_eq(validate(m), tl::unexpected{ValidationError::LocalUndefined});
    });

    s.add_test("Function: valid return", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {Block{.type = {ValueType::Int32}}, I32Const{42}, Return{}, End{}};

        a.expect(validate(m).has_value());
    });

    s.add_test("Function: invalid return, implicit", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {
                Loop{.type = {BlockType::Empty{}}}, I32Const{1}, BranchIf{.label_idx = 0}, End{}};

        a.expect_eq(validate(m), tl::unexpected{ValidationError::ValueStackUnderflow});
    });

    s.add_test("Function: invalid return, explicit", [=](etest::IActions &a) mutable {
        m.code_section->entries[0].code = {
                Loop{.type = {BlockType::Empty{}}}, I32Const{1}, BranchIf{.label_idx = 0}, End{}, Return{}, End{}};

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

    s.add_test("to_string(ValidationError): Every error has a message", [](etest::IActions &a) {
        // This test will fail if we add new first or last errors, but that's fine.
        static constexpr auto kFirstError = ValidationError::BlockTypeInvalid;
        static constexpr auto kLastError = ValidationError::ValueStackUnexpected;

        auto error = static_cast<int>(kFirstError);
        a.expect_eq(error, 0);

        while (error <= static_cast<int>(kLastError)) {
            a.expect(to_string(static_cast<ValidationError>(error)) != "Unknown error",
                    std::to_string(error) + " is missing an error message");
            error += 1;
        }

        a.expect_eq(to_string(static_cast<ValidationError>(error + 1)), "Unknown error");
    });

    s.add_test("Table: valid table", [=](etest::IActions &a) mutable {
        m.table_section = {{{ValueType::FunctionReference, {0, 1}}}};
        a.expect(validate(m).has_value());
    });

    s.add_test("Table: invalid table, min size > max", [=](etest::IActions &a) mutable {
        m.table_section = {{{ValueType::FunctionReference, {1, 0}}}};
        a.expect_eq(validate(m), tl::unexpected{ValidationError::TableInvalid});
    });

    s.add_test("Memory: valid memory", [=](etest::IActions &a) mutable {
        m.memory_section = {{{0, 100}}};
        a.expect(validate(m).has_value());
    });

    s.add_test("Memory: invalid memory, min size > max", [=](etest::IActions &a) mutable {
        m.memory_section = {{{1, 0}}};
        a.expect_eq(validate(m), tl::unexpected{ValidationError::MemoryInvalid});
    });

    s.add_test("Memory: invalid memory, max size > 2^16", [=](etest::IActions &a) mutable {
        m.memory_section = {{{0, 1UL << 17}}};
        a.expect_eq(validate(m), tl::unexpected{ValidationError::MemoryInvalid});
    });

    s.add_test("Global: empty global", [=](etest::IActions &a) mutable {
        m.global_section = GlobalSection{
                .globals = std::vector{Global{
                        .type = GlobalType{.type = ValueType::Int32, .mutability = GlobalType::Mutability::Const},
                        .init = {}}}};

        a.expect(validate(m).has_value());
    });

    s.add_test("Global: initialized global", [=](etest::IActions &a) mutable {
        m.global_section = GlobalSection{
                .globals = std::vector{Global{
                        .type = GlobalType{.type = ValueType::Int32, .mutability = GlobalType::Mutability::Const},
                        .init = {I32Const{42}}}}};

        a.expect(validate(m).has_value());
    });

    s.add_test("Global: initialized global, non-const initializer", [=](etest::IActions &a) mutable {
        m.global_section = GlobalSection{
                .globals = std::vector{Global{
                        .type = GlobalType{.type = ValueType::Int32, .mutability = GlobalType::Mutability::Const},
                        .init = {I32Const{42}, I32Const{42}, I32Add{}}}}};

        a.expect_eq(validate(m), tl::unexpected{ValidationError::GlobalNotConstant});
    });

    s.add_test("Data: passive data", [=](etest::IActions &a) mutable {
        m.data_section = {{DataSection::PassiveData{{std::byte{0x42}}}}};

        a.expect(validate(m).has_value());
    });

    s.add_test("Data: valid active data", [=](etest::IActions &a) mutable {
        m.memory_section = MemorySection{.memories = {MemType{.min = 42}}};
        m.data_section = {{DataSection::ActiveData{0, {I32Const{42}}, {std::byte{0x42}}}}};

        a.expect(validate(m).has_value());
    });

    s.add_test("Data: active data, undefined memory section", [=](etest::IActions &a) mutable {
        m.data_section = {{DataSection::ActiveData{0, {I32Const{42}}, {std::byte{0x42}}}}};

        a.expect_eq(validate(m), tl::unexpected{ValidationError::MemorySectionUndefined});
    });

    s.add_test("Data: active data, non-constant offset", [=](etest::IActions &a) mutable {
        m.memory_section = MemorySection{.memories = {MemType{.min = 42}}};
        m.data_section = {{DataSection::ActiveData{0, {I32Const{42}, I32Const{42}, I32Add{}}, {std::byte{0x42}}}}};

        a.expect_eq(validate(m), tl::unexpected{ValidationError::DataOffsetNotConstant});
    });

    // TODO(dzero): Uncomment this test when we've implemented I64 instructions
    // s.add_test("Data: active data, offset return incorrect type", [=](etest::IActions &a) mutable {
    //    m.memory_section = MemorySection{.memories = {MemType{.min = 42}}};
    //    m.data_section = {{DataSection::ActiveData{0, {I64Const{42}}, {std::byte{0x42}}}}};

    //    a.expect_eq(validate(m), tl::unexpected{ValidationError::DataOffsetNotConstant});
    //});

    s.add_test("Data: active data, invalid memory index", [=](etest::IActions &a) mutable {
        m.memory_section = MemorySection{.memories = {MemType{.min = 42}}};
        m.data_section = {{DataSection::ActiveData{1, {I32Const{42}}, {std::byte{0x42}}}}};

        a.expect_eq(validate(m), tl::unexpected{ValidationError::DataMemoryIdxInvalid});
    });

    s.add_test("Start: valid start function", [=](etest::IActions &a) mutable {
        m.start_section = {0};
        m.function_section = {{0}};
        m.type_section = {{{{}, {}}}};

        a.expect(validate(m).has_value());
    });

    s.add_test("Start: invalid function type", [=](etest::IActions &a) mutable {
        m.start_section = {0};
        m.function_section = {{0}};
        m.type_section = {{{{ValueType::Int32}, {ValueType::Int32}}}};

        a.expect_eq(validate(m), tl::unexpected{ValidationError::StartFunctionTypeInvalid});
    });

    s.add_test("Start: undefined function section", [=](etest::IActions &a) mutable {
        m.start_section = {0};
        m.function_section.reset();
        m.type_section = {{{{}, {}}}};

        a.expect_eq(validate(m), tl::unexpected{ValidationError::FunctionSectionUndefined});
    });

    s.add_test("Start: undefined type section", [=](etest::IActions &a) mutable {
        m.start_section = {0};
        m.function_section = {{0}};
        m.type_section.reset();

        a.expect_eq(validate(m), tl::unexpected{ValidationError::TypeSectionUndefined});
    });

    s.add_test("Start: invalid function index", [=](etest::IActions &a) mutable {
        m.start_section = {1};
        m.function_section = {{0}};
        m.type_section = {{{{}, {}}}};

        a.expect_eq(validate(m), tl::unexpected{ValidationError::StartFunctionInvalid});
    });

    return s.run();
}
