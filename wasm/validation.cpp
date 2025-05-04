// SPDX-FileCopyrightText: 2024-2025 David Zero <zero-one@zer0-one.net>
// SPDX-FileCopyrightText: 2024-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "wasm/validation.h"

#include "util/variant.h"
#include "wasm/instructions.h"
#include "wasm/types.h"
#include "wasm/wasm.h"

#include <tl/expected.hpp>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace wasm::validation {
namespace {

using namespace wasm::instructions;

using Unknown = std::monostate;
using ValueOrUnknown = std::variant<Unknown, ValueType>;

// https://webassembly.github.io/spec/core/valid/types.html#block-types
constexpr bool is_valid(wasm::instructions::BlockType const &bt, Module const &m) {
    if (auto const *t = std::get_if<TypeIdx>(&bt.value)) {
        if (!m.type_section.has_value()) {
            return false;
        }

        if (*t >= m.type_section->types.size()) {
            // Index outside bounds of defined types
            return false;
        }
    }

    return true;
}

// https://webassembly.github.io/spec/core/valid/types.html#limits
constexpr bool is_valid(Limits const &l, std::uint64_t k) {
    if (l.min > k) {
        return false;
    }

    if (l.max.has_value()) {
        if (l.max > k || l.max < l.min) {
            return false;
        }
    }

    return true;
}

// https://webassembly.github.io/spec/core/valid/types.html#table-types
constexpr bool is_valid(TableType const &t) {
    return is_valid(t.limits, (1ULL << 32) - 1);
}

// https://webassembly.github.io/spec/core/valid/types.html#memory-types
constexpr bool is_valid(MemType const &mt) {
    return is_valid(mt, 1ULL << 16);
}

// TODO(Zer0-One): Start using these?
// NOLINTNEXTLINE(readability-avoid-unconditional-preprocessor-if)
#if 0

// https://webassembly.github.io/spec/core/valid/types.html#match-limits
// https://webassembly.github.io/spec/core/valid/types.html#memories
constexpr bool is_match(Limits const &l1, Limits const &l2) {
    if (l1.min >= l2.min) {
        if (!l2.max.has_value()) {
            return true;
        }

        if (l1.max.has_value() && l1.max <= l2.max) {
            return true;
        }
    }

    return false;
}

// https://webassembly.github.io/spec/core/valid/types.html#functions
constexpr bool is_match(FunctionType const &f1, FunctionType const &f2) {
    return f1 == f2;
}

// https://webassembly.github.io/spec/core/valid/types.html#tables
constexpr bool is_match(TableType const &t1, TableType const &t2) {
    return is_match(t1.limits, t2.limits) && t1.element_type == t2.element_type;
}

// https://webassembly.github.io/spec/core/valid/types.html#globals
constexpr bool is_match(GlobalType const &g1, GlobalType const &g2) {
    return g1 == g2;
}
#endif

// https://webassembly.github.io/spec/core/valid/instructions.html#constant-expressions
bool is_constant_expression(std::vector<Instruction> const &expr) {
    return std::ranges::all_of(expr.begin(), expr.end(), [](Instruction inst) {
        // TODO(dzero): Update this with the other const instructions,
        // ref.null, ref.func, and global.get when we implement those
        return util::holds_any_of<I32Const>(inst);
    });
}

// https://webassembly.github.io/spec/core/appendix/algorithm.html#validation-algorithm
struct ControlFrame {
    Instruction i;
    std::vector<ValueType> params;
    std::vector<ValueType> results;
    std::size_t stack_height = 0;
    bool unreachable = false;
};

struct InstValidator {
    std::vector<ValueOrUnknown> value_stack; // operand stack
    std::vector<ControlFrame> control_stack;

    void push_val(ValueOrUnknown const &);
    [[nodiscard]] tl::expected<ValueOrUnknown, ValidationError> pop_val();
    [[nodiscard]] tl::expected<ValueOrUnknown, ValidationError> pop_val_expect(ValueOrUnknown const &);
    void push_vals(std::vector<ValueType> const &);
    [[nodiscard]] tl::expected<std::vector<ValueOrUnknown>, ValidationError> pop_vals(std::vector<ValueType> const &);
    void push_ctrl(Instruction, std::vector<ValueType>, std::vector<ValueType>);
    [[nodiscard]] tl::expected<ControlFrame, ValidationError> pop_ctrl();
    std::vector<ValueType> const &label_types(ControlFrame const &);
    void mark_unreachable();
};

void InstValidator::push_val(ValueOrUnknown const &val) {
    value_stack.push_back(val);
}

tl::expected<ValueOrUnknown, ValidationError> InstValidator::pop_val() {
    assert(!control_stack.empty());

    if (value_stack.size() == control_stack.back().stack_height && control_stack.back().unreachable) {
        return Unknown{};
    }

    if (value_stack.size() == control_stack.back().stack_height) {
        return tl::unexpected{ValidationError::ValueStackUnderflow};
    }

    assert(!value_stack.empty());

    auto val = value_stack.back();

    value_stack.pop_back();

    return val;
}

tl::expected<ValueOrUnknown, ValidationError> InstValidator::pop_val_expect(ValueOrUnknown const &expected) {
    auto actual = pop_val();

    if (!actual.has_value()) {
        return tl::unexpected{actual.error()};
    }

    if (actual != expected && !std::holds_alternative<Unknown>(*actual) && !std::holds_alternative<Unknown>(expected)) {
        return tl::unexpected{ValidationError::ValueStackUnexpected};
    }

    return actual;
}

void InstValidator::push_vals(std::vector<ValueType> const &vals) {
    value_stack.reserve(value_stack.size() + vals.size());

    for (ValueType const &val : vals) {
        value_stack.emplace_back(val);
    }
}

tl::expected<std::vector<ValueOrUnknown>, ValidationError> InstValidator::pop_vals(std::vector<ValueType> const &vals) {
    std::vector<ValueOrUnknown> popped;

    // TODO(dzero): switch to std::ranges::reverse_view once we drop older toolchains
    // NOLINTNEXTLINE(modernize-loop-convert)
    for (auto it = vals.rbegin(); it != vals.rend(); it++) {
        auto maybe_val = pop_val_expect(*it);

        if (!maybe_val.has_value()) {
            return tl::unexpected{maybe_val.error()};
        }

        popped.insert(popped.begin(), *maybe_val);
    }

    return popped;
}

void InstValidator::push_ctrl(Instruction i, std::vector<ValueType> params, std::vector<ValueType> results) {
    if (!params.empty()) {
        push_vals(params);
    }

    control_stack.emplace_back(std::move(i), std::move(params), std::move(results), value_stack.size(), false);
}

tl::expected<ControlFrame, ValidationError> InstValidator::pop_ctrl() {
    if (control_stack.empty()) {
        return tl::unexpected{ValidationError::ControlStackEmpty};
    }

    auto frame = control_stack.back();

    if (!frame.results.empty()) {
        auto maybe_vals = pop_vals(frame.results);

        if (!maybe_vals.has_value()) {
            return tl::unexpected{maybe_vals.error()};
        }
    }

    if (value_stack.size() != frame.stack_height) {
        return tl::unexpected{ValidationError::ValueStackHeightMismatch};
    }

    control_stack.pop_back();

    return frame;
}

std::vector<ValueType> const &InstValidator::label_types(ControlFrame const &frame) {
    if (std::holds_alternative<Loop>(frame.i)) {
        return frame.params;
    }

    return frame.results;
}

void InstValidator::mark_unreachable() {
    value_stack.resize(control_stack.back().stack_height);

    control_stack.back().unreachable = true;
}

tl::expected<void, ValidationError> validate_constant_expression(
        std::vector<Instruction> const &expr, ValueType result) {
    if (expr.empty()) {
        return {};
    }

    InstValidator v;

    v.push_ctrl(Block{}, {}, {result});

    // TODO(dzero): Add other const expression instructions when implemented
    for (auto const inst : expr) {
        if (std::holds_alternative<I32Const>(inst)) {
            v.push_val(ValueType::Int32);
        }
    }

    tl::expected maybe_vals = v.pop_vals(v.label_types(v.control_stack[0]));

    if (!maybe_vals.has_value()) {
        return tl::unexpected{maybe_vals.error()};
    }

    return {};
}

// TODO(dzero): Serialize operand stack and control stack as part of the ValidationError to make debugging easier
// https://webassembly.github.io/spec/core/valid/instructions.html#instruction-sequences
tl::expected<void, ValidationError> validate_function(std::uint32_t func_idx,
        Module const &m,
        FunctionSection const &fs,
        TypeSection const &ts,
        CodeSection const &cs) {
    FunctionType const &func_type = ts.types[fs.type_indices[func_idx]];
    CodeEntry const &func_code = cs.entries[func_idx];

    // https://webassembly.github.io/spec/core/valid/instructions.html#empty-instruction-sequence-epsilon
    if (func_code.code.empty()) {
        return {};
    }

    InstValidator v;

    v.push_ctrl(Block{}, func_type.parameters, func_type.results);

    for (auto inst : func_code.code) {
        assert(!v.control_stack.empty());

        // https://webassembly.github.io/spec/core/valid/instructions.html#numeric-instructions
        // constant
        if (std::holds_alternative<I32Const>(inst)) {
            v.push_val(ValueType::Int32);
        }
        // cvtop
        // TODO(dzero): figure out what to do with i32.extend8_s and i32.extend16_s
        else if (util::holds_any_of<I32WrapI64>(inst)) {
            auto maybe_val = v.pop_val_expect(ValueType::Int64);

            if (!maybe_val.has_value()) {
                return tl::unexpected{maybe_val.error()};
            }

            v.push_val(ValueType::Int32);
        } else if (util::holds_any_of<I32TruncateF32Signed, I32TruncateF32Unsigned, I32ReinterpretF32>(inst)) {
            auto maybe_val = v.pop_val_expect(ValueType::Float32);

            if (!maybe_val.has_value()) {
                return tl::unexpected{maybe_val.error()};
            }

            v.push_val(ValueType::Int32);
        } else if (util::holds_any_of<I32TruncateF64Signed, I32TruncateF64Unsigned>(inst)) {
            auto maybe_val = v.pop_val_expect(ValueType::Float64);

            if (!maybe_val.has_value()) {
                return tl::unexpected{maybe_val.error()};
            }

            v.push_val(ValueType::Int32);
        }
        // iunop + itestop
        else if (util::holds_any_of<I32CountLeadingZeros, I32CountTrailingZeros, I32PopulationCount, I32EqualZero>(
                         inst)) {
            auto maybe_val = v.pop_val_expect(ValueType::Int32);

            if (!maybe_val.has_value()) {
                return tl::unexpected{maybe_val.error()};
            }

            v.push_val(ValueType::Int32);
        }
        // ibinop + irelop
        else if (util::holds_any_of<I32Add,
                         I32Subtract,
                         I32Multiply,
                         I32DivideSigned,
                         I32DivideUnsigned,
                         I32RemainderSigned,
                         I32RemainderUnsigned,
                         I32And,
                         I32Or,
                         I32ExclusiveOr,
                         I32ShiftLeft,
                         I32ShiftRightSigned,
                         I32ShiftRightUnsigned,
                         I32RotateLeft,
                         I32RotateRight,
                         I32Equal,
                         I32NotEqual,
                         I32LessThanSigned,
                         I32LessThanUnsigned,
                         I32GreaterThanSigned,
                         I32GreaterThanUnsigned,
                         I32LessThanEqualSigned,
                         I32LessThanEqualUnsigned,
                         I32GreaterThanEqualSigned,
                         I32GreaterThanEqualUnsigned>(inst)) {
            auto maybe_val = v.pop_val_expect(ValueType::Int32);

            if (!maybe_val.has_value()) {
                return tl::unexpected{maybe_val.error()};
            }

            maybe_val = v.pop_val_expect(ValueType::Int32);

            if (!maybe_val.has_value()) {
                return tl::unexpected{maybe_val.error()};
            }

            v.push_val(ValueType::Int32);
        }
        // https://webassembly.github.io/spec/core/valid/instructions.html#variable-instructions
        else if (auto const *lg = std::get_if<LocalGet>(&inst)) {
            if (lg->idx >= func_code.locals.size()) {
                return tl::unexpected{ValidationError::LocalUndefined};
            }

            v.push_val(func_code.locals[lg->idx].type);
        } else if (auto const *ls = std::get_if<LocalSet>(&inst)) {
            if (ls->idx >= func_code.locals.size()) {
                return tl::unexpected{ValidationError::LocalUndefined};
            }

            if (auto pop_res = v.pop_val_expect(func_code.locals[ls->idx].type); !pop_res.has_value()) {
                return tl::unexpected{pop_res.error()};
            }
        } else if (auto const *lt = std::get_if<LocalTee>(&inst)) {
            if (lt->idx >= func_code.locals.size()) {
                return tl::unexpected{ValidationError::LocalUndefined};
            }

            if (auto pop_res = v.pop_val_expect(func_code.locals[lt->idx].type); !pop_res.has_value()) {
                return tl::unexpected{pop_res.error()};
            }

            v.push_val(func_code.locals[lt->idx].type);
        }
        // https://webassembly.github.io/spec/core/valid/instructions.html#memory-instructions
        else if (auto const *i32l = std::get_if<I32Load>(&inst)) {
            if (!m.memory_section.has_value()) {
                return tl::unexpected{ValidationError::MemorySectionUndefined};
            }

            if (m.memory_section->memories.empty()) {
                return tl::unexpected{ValidationError::MemoryEmpty};
            }

            if (i32l->arg.align > (32 / 8)) {
                return tl::unexpected{ValidationError::MemoryBadAlignment};
            }

            auto maybe_val = v.pop_val_expect(ValueType::Int32);

            if (!maybe_val.has_value()) {
                return tl::unexpected{maybe_val.error()};
            }

            v.push_val(ValueType::Int32);
        }
        // https://webassembly.github.io/spec/core/valid/instructions.html#control-instructions
        else if (auto const *block = std::get_if<Block>(&inst)) {
            if (!is_valid(block->type, m)) {
                return tl::unexpected{ValidationError::BlockTypeInvalid};
            }

            std::vector<ValueType> params;
            std::vector<ValueType> results;

            if (auto const *vt = std::get_if<ValueType>(&(block->type.value))) {
                results.push_back(*vt);
            } else if (auto const *idx = std::get_if<TypeIdx>(&(block->type.value))) {
                params = ts.types[*idx].parameters;
                results = ts.types[*idx].results;
            }

            // The case of an empty block type is handled implicitly by leaving the vectors empty

            v.push_ctrl(Block{}, std::move(params), std::move(results));
        } else if (auto const *loop = std::get_if<Loop>(&inst)) {
            if (!is_valid(loop->type, m)) {
                return tl::unexpected{ValidationError::BlockTypeInvalid};
            }

            std::vector<ValueType> params;
            std::vector<ValueType> results;

            if (auto const *vt = std::get_if<ValueType>(&loop->type.value)) {
                results.push_back(*vt);
            } else if (auto const *idx = std::get_if<TypeIdx>(&loop->type.value)) {
                params = ts.types[*idx].parameters;
                results = ts.types[*idx].results;
            }

            // The case of an empty block type is handled implicitly by leaving the vectors empty

            v.push_ctrl(Loop{}, std::move(params), std::move(results));
        } else if (std::holds_alternative<End>(inst)) {
            tl::expected<ControlFrame, ValidationError> maybe_frame = v.pop_ctrl();

            if (!maybe_frame.has_value()) {
                return tl::unexpected{maybe_frame.error()};
            }

            v.push_vals(maybe_frame->results);
        } else if (auto const *branch = std::get_if<Branch>(&inst)) {
            if (v.control_stack.size() <= branch->label_idx) {
                return tl::unexpected{ValidationError::LabelInvalid};
            }

            tl::expected maybe_vals =
                    v.pop_vals(v.label_types(v.control_stack[v.control_stack.size() - (branch->label_idx + 1)]));

            if (!maybe_vals.has_value()) {
                return tl::unexpected{maybe_vals.error()};
            }

            v.mark_unreachable();
        } else if (auto const *branch_if = std::get_if<BranchIf>(&inst)) {
            if (v.control_stack.size() <= branch_if->label_idx) {
                return tl::unexpected{ValidationError::LabelInvalid};
            }

            auto maybe_val = v.pop_val_expect(ValueType::Int32);

            if (!maybe_val.has_value()) {
                return tl::unexpected{maybe_val.error()};
            }

            tl::expected maybe_vals =
                    v.pop_vals(v.label_types(v.control_stack[v.control_stack.size() - (branch_if->label_idx + 1)]));

            if (!maybe_vals.has_value()) {
                return tl::unexpected{maybe_vals.error()};
            }

            v.push_vals(v.label_types(v.control_stack[v.control_stack.size() - (branch_if->label_idx + 1)]));
        } else if (std::holds_alternative<Return>(inst)) {
            tl::expected maybe_vals = v.pop_vals(v.label_types(v.control_stack[0]));

            if (!maybe_vals.has_value()) {
                return tl::unexpected{maybe_vals.error()};
            }

            v.mark_unreachable();
        } else {
            return tl::unexpected{ValidationError::UnknownInstruction};
        }
    }

    // Check function return values, but only if we didn't just execute a
    // return. This only happens if a "return" was the last instruction in the
    // sequence.
    if (!std::holds_alternative<Return>(func_code.code.back()) && !v.control_stack.empty()) {
        tl::expected maybe_vals = v.pop_vals(v.label_types(v.control_stack[0]));

        if (!maybe_vals.has_value()) {
            return tl::unexpected{maybe_vals.error()};
        }
    }

    return {};
}

tl::expected<void, ValidationError> validate_functions(Module const &m, FunctionSection const &fs) {
    if (!m.type_section.has_value()) {
        return tl::unexpected{ValidationError::TypeSectionUndefined};
    }

    if (!m.code_section.has_value()) {
        return tl::unexpected{ValidationError::CodeSectionUndefined};
    }

    for (std::uint32_t i = 0; i < fs.type_indices.size(); i++) {
        if (fs.type_indices[i] >= m.type_section->types.size()) {
            return tl::unexpected{ValidationError::FuncTypeInvalid};
        }

        if (i >= m.code_section->entries.size()) {
            return tl::unexpected{ValidationError::FuncUndefinedCode};
        }

        auto const ret = validate_function(i, m, fs, *m.type_section, *m.code_section);

        if (!ret.has_value()) {
            return ret;
        }
    }

    return {};
}

} // namespace

std::string_view to_string(ValidationError err) {
    switch (err) {
        case ValidationError::BlockTypeInvalid:
            return "BlockType of a block or loop is invalid; the type section is undefined, or the type index was "
                   "out-of-bounds.";
        case ValidationError::CodeSectionUndefined:
            return "A code section is required, but was not defined";
        case ValidationError::ControlStackEmpty:
            return "Attempted to pop from the control stack, but the control stack is empty";
        case ValidationError::DataOffsetNotConstant:
            return "A data offset was specified with a non-constant expression";
        case ValidationError::DataMemoryIdxInvalid:
            return "A data memory index points to a non-existent memory";
        case ValidationError::FuncTypeInvalid:
            return "Function section references a non-existent type";
        case ValidationError::FunctionSectionUndefined:
            return "A function section is required, but was not defined";
        case ValidationError::FuncUndefinedCode:
            return "Function body is undefined/missing";
        case ValidationError::GlobalNotConstant:
            return "A global is being initialized with a non-constant expression";
        case ValidationError::LabelInvalid:
            return "Attempted to branch to a label which isn't valid";
        case ValidationError::LocalUndefined:
            return "Attempted to index a local which isn't defined in the current code entry";
        case ValidationError::MemoryBadAlignment:
            return "Attempted a load or store with a bad alignment value";
        case ValidationError::MemoryEmpty:
            return "Attempted a load, but memory is empty";
        case ValidationError::MemoryInvalid:
            return "A memory has invalid limits";
        case ValidationError::MemorySectionUndefined:
            return "Attempted a load/store or data initialization, but no memory section was defined";
        case ValidationError::StartFunctionInvalid:
            return "Start section references a non-existent function";
        case ValidationError::StartFunctionTypeInvalid:
            return "Start function references a non-existent or invalid type";
        case ValidationError::TableInvalid:
            return "A table has invalid limits";
        case ValidationError::TypeSectionUndefined:
            return "A type section is required, but was not defined";
        case ValidationError::UnknownInstruction:
            return "Unknown instruction encountered";
        case ValidationError::ValueStackHeightMismatch:
            return "Value stack height on exiting a control frame does not match the height on entry";
        case ValidationError::ValueStackUnderflow:
            return "Attempted to pop from the value stack, but stack height would underflow";
        case ValidationError::ValueStackUnexpected:
            return "Attempted to pop an expected value from the value stack, but got a different value";
    }

    return "Unknown error";
}

// https://webassembly.github.io/spec/core/valid/modules.html#modules
tl::expected<void, ValidationError> validate(Module const &m) {
    // https://webassembly.github.io/spec/core/valid/modules.html#functions
    if (m.function_section.has_value()) {
        auto const ret = validate_functions(m, *m.function_section);

        if (!ret.has_value()) {
            return ret;
        }
    }

    // https://webassembly.github.io/spec/core/valid/modules.html#tables
    if (m.table_section.has_value()) {
        for (auto const &t : m.table_section->tables) {
            if (!is_valid(t)) {
                return tl::unexpected{ValidationError::TableInvalid};
            }
        }
    }

    // https://webassembly.github.io/spec/core/valid/modules.html#memories
    if (m.memory_section.has_value()) {
        for (auto const &mem : m.memory_section->memories) {
            if (!is_valid(mem)) {
                return tl::unexpected{ValidationError::MemoryInvalid};
            }
        }
    }

    // https://webassembly.github.io/spec/core/valid/modules.html#globals
    if (m.global_section.has_value()) {
        for (auto const &global : m.global_section->globals) {
            if (!is_constant_expression(global.init)) {
                return tl::unexpected{ValidationError::GlobalNotConstant};
            }

            auto const ret = validate_constant_expression(global.init, global.type.type);

            if (!ret.has_value()) {
                return ret;
            }
        }
    }

    // https://webassembly.github.io/spec/core/valid/modules.html#data-segments
    if (m.data_section.has_value()) {
        for (auto const &data : m.data_section->data) {
            // Passive data is valid by default, so we only check active
            if (auto const *dat = std::get_if<DataSection::ActiveData>(&data)) {
                if (!is_constant_expression(dat->offset)) {
                    return tl::unexpected{ValidationError::DataOffsetNotConstant};
                }

                auto const ret = validate_constant_expression(dat->offset, ValueType::Int32);

                if (!ret.has_value()) {
                    return ret;
                }

                if (!m.memory_section.has_value()) {
                    return tl::unexpected{ValidationError::MemorySectionUndefined};
                }

                if (dat->memory_idx >= m.memory_section->memories.size()) {
                    return tl::unexpected{ValidationError::DataMemoryIdxInvalid};
                }
            }
        }
    }

    // https://webassembly.github.io/spec/core/valid/modules.html#start-function
    if (m.start_section.has_value()) {
        // Ensure function and type sections exist
        if (!m.function_section.has_value()) {
            return tl::unexpected{ValidationError::FunctionSectionUndefined};
        }

        if (!m.type_section.has_value()) {
            return tl::unexpected{ValidationError::TypeSectionUndefined};
        }

        // Ensure start function has valid index into function and type section
        if (m.start_section->start >= m.function_section->type_indices.size()) {
            return tl::unexpected{ValidationError::StartFunctionInvalid};
        }

        if (m.start_section->start >= m.type_section->types.size()) {
            return tl::unexpected{ValidationError::StartFunctionTypeInvalid};
        }

        // Start function must have type [] -> []
        if (!m.type_section->types[m.start_section->start].parameters.empty()
                || !m.type_section->types[m.start_section->start].results.empty()) {
            return tl::unexpected{ValidationError::StartFunctionTypeInvalid};
        }
    }

    return {};
}

} // namespace wasm::validation
