// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef WASM_INSTRUCTIONS_H_
#define WASM_INSTRUCTIONS_H_

#include "wasm/wasm.h"

#include <cstdint>
#include <iosfwd>
#include <optional>
#include <variant>
#include <vector>

namespace wasm::instructions {

struct BlockType {
    static std::optional<BlockType> parse(std::istream &);

    struct Empty {
        [[nodiscard]] bool operator==(Empty const &) const = default;
    };
    std::variant<Empty, ValueType, TypeIdx> value;
    [[nodiscard]] bool operator==(BlockType const &) const = default;
};

struct MemArg {
    static std::optional<MemArg> parse(std::istream &);

    std::uint32_t align{};
    std::uint32_t offset{};
    [[nodiscard]] bool operator==(MemArg const &) const = default;
};

struct Block;
struct Loop;
struct BreakIf;
struct Return;

struct I32Const;
struct I32LessThanSigned;
struct I32Add;

struct LocalGet;
struct LocalSet;
struct LocalTee;

struct I32Load;

using Instruction = std::variant<Block,
        Loop,
        BreakIf,
        Return,
        I32Const,
        I32LessThanSigned,
        I32Add,
        LocalGet,
        LocalSet,
        LocalTee,
        I32Load>;

// https://webassembly.github.io/spec/core/binary/instructions.html#control-instructions
struct Block {
    static constexpr std::uint8_t kOpcode = 0x02;
    BlockType type{};
    std::vector<Instruction> instructions;
    [[nodiscard]] bool operator==(Block const &) const;
};

struct Loop {
    static constexpr std::uint8_t kOpcode = 0x03;
    BlockType type{};
    std::vector<Instruction> instructions;
    [[nodiscard]] bool operator==(Loop const &) const;
};

struct BreakIf {
    static constexpr std::uint8_t kOpcode = 0x0d;
    std::uint32_t label_idx{};
    [[nodiscard]] bool operator==(BreakIf const &) const = default;
};

struct Return {
    static constexpr std::uint8_t kOpcode = 0x0f;
    [[nodiscard]] bool operator==(Return const &) const = default;
};

struct End {
    static constexpr std::uint8_t kOpcode = 0x0b;
    [[nodiscard]] bool operator==(End const &) const = default;
};

// https://webassembly.github.io/spec/core/binary/instructions.html#numeric-instructions
struct I32Const {
    static constexpr std::uint8_t kOpcode = 0x41;
    std::int32_t value{};
    [[nodiscard]] bool operator==(I32Const const &) const = default;
};

struct I32LessThanSigned {
    static constexpr std::uint8_t kOpcode = 0x48;
    [[nodiscard]] bool operator==(I32LessThanSigned const &) const = default;
};

struct I32Add {
    static constexpr std::uint8_t kOpcode = 0x6a;
    [[nodiscard]] bool operator==(I32Add const &) const = default;
};

// https://webassembly.github.io/spec/core/binary/instructions.html#variable-instructions
struct LocalGet {
    static constexpr std::uint8_t kOpcode = 0x20;
    std::uint32_t idx{};
    [[nodiscard]] bool operator==(LocalGet const &) const = default;
};

struct LocalSet {
    static constexpr std::uint8_t kOpcode = 0x21;
    std::uint32_t idx{};
    [[nodiscard]] bool operator==(LocalSet const &) const = default;
};

struct LocalTee {
    static constexpr std::uint8_t kOpcode = 0x22;
    std::uint32_t idx{};
    [[nodiscard]] bool operator==(LocalTee const &) const = default;
};

// https://webassembly.github.io/spec/core/binary/instructions.html#memory-instructions
struct I32Load {
    static constexpr std::uint8_t kOpcode = 0x28;
    MemArg arg{};
    [[nodiscard]] bool operator==(I32Load const &) const = default;
};
std::optional<std::vector<Instruction>> parse(std::istream &);

} // namespace wasm::instructions

#endif
