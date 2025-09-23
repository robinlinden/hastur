// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef WASM_TYPES_H_
#define WASM_TYPES_H_

#include <cstdint>
#include <optional>
#include <vector>

namespace wasm {

// https://webassembly.github.io/spec/core/binary/modules.html#indices
using TypeIdx = std::uint32_t;
using FuncIdx = std::uint32_t;

// https://webassembly.github.io/spec/core/syntax/types.html
enum class ValueType : std::uint8_t {
    // Number types.
    Int32,
    Int64,
    Float32,
    Float64,

    // Vector types.
    Vector128,

    // Reference types.
    FunctionReference,
    ExternReference,
};

// https://webassembly.github.io/spec/core/binary/types.html#result-types
using ResultType = std::vector<ValueType>;

// https://webassembly.github.io/spec/core/binary/types.html#function-types
struct FunctionType {
    ResultType parameters;
    ResultType results;

    [[nodiscard]] bool operator==(FunctionType const &) const = default;
};

// https://webassembly.github.io/spec/core/binary/types.html#limits
struct Limits {
    std::uint32_t min{};
    std::optional<std::uint32_t> max;

    [[nodiscard]] bool operator==(Limits const &) const = default;
};

// https://webassembly.github.io/spec/core/binary/types.html#table-types
struct TableType {
    ValueType element_type{};
    Limits limits{};

    [[nodiscard]] bool operator==(TableType const &) const = default;
};

// https://webassembly.github.io/spec/core/binary/types.html#memory-types
using MemType = Limits;

} // namespace wasm

#endif
