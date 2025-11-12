// SPDX-FileCopyrightText: 2023-2025 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2024 David Zero <zero-one@zer0-one.net>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef WASM_INSTRUCTIONS_H_
#define WASM_INSTRUCTIONS_H_

#include "wasm/types.h"

#include <cstdint>
#include <functional>
#include <string_view>
#include <variant>

namespace wasm::instructions {

// https://webassembly.github.io/spec/core/exec/instructions.html#numeric-instructions
enum class NumericType : std::uint8_t {
    // TODO(robinlinden): cvtop.
    Binop,
    Const,
    Relop,
    Testop,
    Unop,
};

struct BlockType {
    struct Empty {
        [[nodiscard]] bool operator==(Empty const &) const = default;
    };
    std::variant<Empty, ValueType, TypeIdx> value;
    [[nodiscard]] bool operator==(BlockType const &) const = default;
};

struct MemArg {
    std::uint32_t align{};
    std::uint32_t offset{};

    [[nodiscard]] bool operator==(MemArg const &) const = default;
};

// Control instructions
struct Block;
struct Loop;
struct Branch;
struct BranchIf;
struct Return;
struct End;

// Numeric instructions
struct I32Const;
struct I32EqualZero;
struct I32Equal;
struct I32NotEqual;
struct I32LessThanSigned;
struct I32LessThanUnsigned;
struct I32GreaterThanSigned;
struct I32GreaterThanUnsigned;
struct I32LessThanEqualSigned;
struct I32LessThanEqualUnsigned;
struct I32GreaterThanEqualSigned;
struct I32GreaterThanEqualUnsigned;
struct I32CountLeadingZeros;
struct I32CountTrailingZeros;
struct I32PopulationCount;
struct I32Add;
struct I32Subtract;
struct I32Multiply;
struct I32DivideSigned;
struct I32DivideUnsigned;
struct I32RemainderSigned;
struct I32RemainderUnsigned;
struct I32And;
struct I32Or;
struct I32ExclusiveOr;
struct I32ShiftLeft;
struct I32ShiftRightSigned;
struct I32ShiftRightUnsigned;
struct I32RotateLeft;
struct I32RotateRight;
struct I32WrapI64;
struct I32TruncateF32Signed;
struct I32TruncateF32Unsigned;
struct I32TruncateF64Signed;
struct I32TruncateF64Unsigned;
struct I32ReinterpretF32;
struct I32Extend8Signed;
struct I32Extend16Signed;

// Variable instructions
struct LocalGet;
struct LocalSet;
struct LocalTee;

// Memory instructions
struct I32Load;

using Instruction = std::variant<Block,
        Loop,
        Branch,
        BranchIf,
        Return,
        End,
        I32Const,
        I32EqualZero,
        I32Equal,
        I32NotEqual,
        I32LessThanSigned,
        I32LessThanUnsigned,
        I32GreaterThanSigned,
        I32GreaterThanUnsigned,
        I32LessThanEqualSigned,
        I32LessThanEqualUnsigned,
        I32GreaterThanEqualSigned,
        I32GreaterThanEqualUnsigned,
        I32CountLeadingZeros,
        I32CountTrailingZeros,
        I32PopulationCount,
        I32Add,
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
        I32WrapI64,
        I32TruncateF32Signed,
        I32TruncateF32Unsigned,
        I32TruncateF64Signed,
        I32TruncateF64Unsigned,
        I32ReinterpretF32,
        I32Extend8Signed,
        I32Extend16Signed,
        LocalGet,
        LocalSet,
        LocalTee,
        I32Load>;

// https://webassembly.github.io/spec/core/binary/instructions.html#control-instructions
struct Block {
    static constexpr std::uint8_t kOpcode = 0x02;
    static constexpr std::string_view kMnemonic = "block";
    BlockType type{};
    [[nodiscard]] bool operator==(Block const &) const = default;
};

struct Loop {
    static constexpr std::uint8_t kOpcode = 0x03;
    static constexpr std::string_view kMnemonic = "loop";
    BlockType type{};
    [[nodiscard]] bool operator==(Loop const &) const = default;
};

struct Branch {
    static constexpr std::uint8_t kOpcode = 0x0c;
    static constexpr std::string_view kMnemonic = "br";
    std::uint32_t label_idx{};
    [[nodiscard]] bool operator==(Branch const &) const = default;
};

struct BranchIf {
    static constexpr std::uint8_t kOpcode = 0x0d;
    static constexpr std::string_view kMnemonic = "br_if";
    std::uint32_t label_idx{};
    [[nodiscard]] bool operator==(BranchIf const &) const = default;
};

struct Return {
    static constexpr std::uint8_t kOpcode = 0x0f;
    static constexpr std::string_view kMnemonic = "return";
    [[nodiscard]] bool operator==(Return const &) const = default;
};

struct End {
    static constexpr std::uint8_t kOpcode = 0x0b;
    static constexpr std::string_view kMnemonic = "end";
    [[nodiscard]] bool operator==(End const &) const = default;
};

// https://webassembly.github.io/spec/core/binary/instructions.html#numeric-instructions
struct I32Const {
    static constexpr std::uint8_t kOpcode = 0x41;
    static constexpr std::string_view kMnemonic = "i32.const";
    std::int32_t value{};
    [[nodiscard]] bool operator==(I32Const const &) const = default;
};

struct I32EqualZero {
    static constexpr std::uint8_t kOpcode = 0x45;
    static constexpr std::string_view kMnemonic = "i32.eqz";
    [[nodiscard]] bool operator==(I32EqualZero const &) const = default;
};

struct I32Equal {
    static constexpr std::uint8_t kOpcode = 0x46;
    static constexpr std::string_view kMnemonic = "i32.eq";
    [[nodiscard]] bool operator==(I32Equal const &) const = default;
};

struct I32NotEqual {
    static constexpr std::uint8_t kOpcode = 0x47;
    static constexpr std::string_view kMnemonic = "i32.ne";
    [[nodiscard]] bool operator==(I32NotEqual const &) const = default;
};

struct I32LessThanSigned {
    static constexpr std::uint8_t kOpcode = 0x48;
    static constexpr std::string_view kMnemonic = "i32.lt_s";

    static constexpr NumericType kNumericType = NumericType::Relop;
    using NumType = std::int32_t;
    using Operation = std::less<std::int32_t>;
    [[nodiscard]] bool operator==(I32LessThanSigned const &) const = default;
};

struct I32LessThanUnsigned {
    static constexpr std::uint8_t kOpcode = 0x49;
    static constexpr std::string_view kMnemonic = "i32.lt_u";
    [[nodiscard]] bool operator==(I32LessThanUnsigned const &) const = default;
};

struct I32GreaterThanSigned {
    static constexpr std::uint8_t kOpcode = 0x4a;
    static constexpr std::string_view kMnemonic = "i32.gt_s";

    static constexpr NumericType kNumericType = NumericType::Relop;
    using NumType = std::int32_t;
    using Operation = std::greater<std::int32_t>;
    [[nodiscard]] bool operator==(I32GreaterThanSigned const &) const = default;
};

struct I32GreaterThanUnsigned {
    static constexpr std::uint8_t kOpcode = 0x4b;
    static constexpr std::string_view kMnemonic = "i32.gt_u";
    [[nodiscard]] bool operator==(I32GreaterThanUnsigned const &) const = default;
};

struct I32LessThanEqualSigned {
    static constexpr std::uint8_t kOpcode = 0x4c;
    static constexpr std::string_view kMnemonic = "i32.le_s";

    static constexpr NumericType kNumericType = NumericType::Relop;
    using NumType = std::int32_t;
    using Operation = std::less_equal<std::int32_t>;
    [[nodiscard]] bool operator==(I32LessThanEqualSigned const &) const = default;
};

struct I32LessThanEqualUnsigned {
    static constexpr std::uint8_t kOpcode = 0x4d;
    static constexpr std::string_view kMnemonic = "i32.le_u";
    [[nodiscard]] bool operator==(I32LessThanEqualUnsigned const &) const = default;
};

struct I32GreaterThanEqualSigned {
    static constexpr std::uint8_t kOpcode = 0x4e;
    static constexpr std::string_view kMnemonic = "i32.ge_s";

    static constexpr NumericType kNumericType = NumericType::Relop;
    using NumType = std::int32_t;
    using Operation = std::greater_equal<std::int32_t>;
    [[nodiscard]] bool operator==(I32GreaterThanEqualSigned const &) const = default;
};

struct I32GreaterThanEqualUnsigned {
    static constexpr std::uint8_t kOpcode = 0x4f;
    static constexpr std::string_view kMnemonic = "i32.ge_u";
    [[nodiscard]] bool operator==(I32GreaterThanEqualUnsigned const &) const = default;
};

struct I32CountLeadingZeros {
    static constexpr std::uint8_t kOpcode = 0x67;
    static constexpr std::string_view kMnemonic = "i32.clz";
    [[nodiscard]] bool operator==(I32CountLeadingZeros const &) const = default;
};

struct I32CountTrailingZeros {
    static constexpr std::uint8_t kOpcode = 0x68;
    static constexpr std::string_view kMnemonic = "i32.ctz";
    [[nodiscard]] bool operator==(I32CountTrailingZeros const &) const = default;
};

struct I32PopulationCount {
    static constexpr std::uint8_t kOpcode = 0x69;
    static constexpr std::string_view kMnemonic = "i32.popcnt";
    [[nodiscard]] bool operator==(I32PopulationCount const &) const = default;
};

struct I32Add {
    static constexpr std::uint8_t kOpcode = 0x6a;
    static constexpr std::string_view kMnemonic = "i32.add";
    [[nodiscard]] bool operator==(I32Add const &) const = default;
};

struct I32Subtract {
    static constexpr std::uint8_t kOpcode = 0x6b;
    static constexpr std::string_view kMnemonic = "i32.sub";
    [[nodiscard]] bool operator==(I32Subtract const &) const = default;
};

struct I32Multiply {
    static constexpr std::uint8_t kOpcode = 0x6c;
    static constexpr std::string_view kMnemonic = "i32.mul";
    [[nodiscard]] bool operator==(I32Multiply const &) const = default;
};

struct I32DivideSigned {
    static constexpr std::uint8_t kOpcode = 0x6d;
    static constexpr std::string_view kMnemonic = "i32.div_s";
    [[nodiscard]] bool operator==(I32DivideSigned const &) const = default;
};

struct I32DivideUnsigned {
    static constexpr std::uint8_t kOpcode = 0x6e;
    static constexpr std::string_view kMnemonic = "i32.div_u";
    [[nodiscard]] bool operator==(I32DivideUnsigned const &) const = default;
};

struct I32RemainderSigned {
    static constexpr std::uint8_t kOpcode = 0x6f;
    static constexpr std::string_view kMnemonic = "i32.rem_s";
    [[nodiscard]] bool operator==(I32RemainderSigned const &) const = default;
};

struct I32RemainderUnsigned {
    static constexpr std::uint8_t kOpcode = 0x70;
    static constexpr std::string_view kMnemonic = "i32.rem_u";
    [[nodiscard]] bool operator==(I32RemainderUnsigned const &) const = default;
};

struct I32And {
    static constexpr std::uint8_t kOpcode = 0x71;
    static constexpr std::string_view kMnemonic = "i32.and";
    [[nodiscard]] bool operator==(I32And const &) const = default;
};

struct I32Or {
    static constexpr std::uint8_t kOpcode = 0x72;
    static constexpr std::string_view kMnemonic = "i32.or";
    [[nodiscard]] bool operator==(I32Or const &) const = default;
};

struct I32ExclusiveOr {
    static constexpr std::uint8_t kOpcode = 0x73;
    static constexpr std::string_view kMnemonic = "i32.xor";
    [[nodiscard]] bool operator==(I32ExclusiveOr const &) const = default;
};

struct I32ShiftLeft {
    static constexpr std::uint8_t kOpcode = 0x74;
    static constexpr std::string_view kMnemonic = "i32.shl";
    [[nodiscard]] bool operator==(I32ShiftLeft const &) const = default;
};

struct I32ShiftRightSigned {
    static constexpr std::uint8_t kOpcode = 0x75;
    static constexpr std::string_view kMnemonic = "i32.shr_s";
    [[nodiscard]] bool operator==(I32ShiftRightSigned const &) const = default;
};

struct I32ShiftRightUnsigned {
    static constexpr std::uint8_t kOpcode = 0x76;
    static constexpr std::string_view kMnemonic = "i32.shr_u";
    [[nodiscard]] bool operator==(I32ShiftRightUnsigned const &) const = default;
};

struct I32RotateLeft {
    static constexpr std::uint8_t kOpcode = 0x77;
    static constexpr std::string_view kMnemonic = "i32.rotl";
    [[nodiscard]] bool operator==(I32RotateLeft const &) const = default;
};

struct I32RotateRight {
    static constexpr std::uint8_t kOpcode = 0x78;
    static constexpr std::string_view kMnemonic = "i32.rotr";
    [[nodiscard]] bool operator==(I32RotateRight const &) const = default;
};

struct I32WrapI64 {
    static constexpr std::uint8_t kOpcode = 0xa7;
    static constexpr std::string_view kMnemonic = "i32.wrap_i64";
    [[nodiscard]] bool operator==(I32WrapI64 const &) const = default;
};

struct I32TruncateF32Signed {
    static constexpr std::uint8_t kOpcode = 0xa8;
    static constexpr std::string_view kMnemonic = "i32.trunc_f32_s";
    [[nodiscard]] bool operator==(I32TruncateF32Signed const &) const = default;
};

struct I32TruncateF32Unsigned {
    static constexpr std::uint8_t kOpcode = 0xa9;
    static constexpr std::string_view kMnemonic = "i32.trunc_f32_u";
    [[nodiscard]] bool operator==(I32TruncateF32Unsigned const &) const = default;
};

struct I32TruncateF64Signed {
    static constexpr std::uint8_t kOpcode = 0xaa;
    static constexpr std::string_view kMnemonic = "i32.trunc_f64_s";
    [[nodiscard]] bool operator==(I32TruncateF64Signed const &) const = default;
};

struct I32TruncateF64Unsigned {
    static constexpr std::uint8_t kOpcode = 0xab;
    static constexpr std::string_view kMnemonic = "i32.trunc_f64_u";
    [[nodiscard]] bool operator==(I32TruncateF64Unsigned const &) const = default;
};

struct I32ReinterpretF32 {
    static constexpr std::uint8_t kOpcode = 0xbc;
    static constexpr std::string_view kMnemonic = "i32.reinterpret_f32";
    [[nodiscard]] bool operator==(I32ReinterpretF32 const &) const = default;
};

struct I32Extend8Signed {
    static constexpr std::uint8_t kOpcode = 0xc0;
    static constexpr std::string_view kMnemonic = "i32.extend8_s";
    [[nodiscard]] bool operator==(I32Extend8Signed const &) const = default;
};

struct I32Extend16Signed {
    static constexpr std::uint8_t kOpcode = 0xc1;
    static constexpr std::string_view kMnemonic = "i32.extend16_s";
    [[nodiscard]] bool operator==(I32Extend16Signed const &) const = default;
};

// https://webassembly.github.io/spec/core/binary/instructions.html#variable-instructions
struct LocalGet {
    static constexpr std::uint8_t kOpcode = 0x20;
    static constexpr std::string_view kMnemonic = "local.get";
    std::uint32_t idx{};
    [[nodiscard]] bool operator==(LocalGet const &) const = default;
};

struct LocalSet {
    static constexpr std::uint8_t kOpcode = 0x21;
    static constexpr std::string_view kMnemonic = "local.set";
    std::uint32_t idx{};
    [[nodiscard]] bool operator==(LocalSet const &) const = default;
};

struct LocalTee {
    static constexpr std::uint8_t kOpcode = 0x22;
    static constexpr std::string_view kMnemonic = "local.tee";
    std::uint32_t idx{};
    [[nodiscard]] bool operator==(LocalTee const &) const = default;
};

// https://webassembly.github.io/spec/core/binary/instructions.html#memory-instructions
struct I32Load {
    static constexpr std::uint8_t kOpcode = 0x28;
    static constexpr std::string_view kMnemonic = "i32.load";
    MemArg arg{};
    [[nodiscard]] bool operator==(I32Load const &) const = default;
};

} // namespace wasm::instructions

#endif
