// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef WASM_WASM_H_
#define WASM_WASM_H_

#include <cstdint>
#include <iosfwd>
#include <optional>
#include <string>
#include <vector>

#include <tl/expected.hpp>

namespace wasm {

// https://webassembly.github.io/spec/core/binary/modules.html#indices
using TypeIdx = std::uint32_t;

// https://webassembly.github.io/spec/core/binary/modules.html#sections
enum class SectionId {
    Custom = 0,
    Type = 1,
    Import = 2,
    Function = 3,
    Table = 4,
    Memory = 5,
    Global = 6,
    Export = 7,
    Start = 8,
    Element = 9,
    Code = 10,
    Data = 11,
    DataCount = 12,
};

struct Section {
    SectionId id{};
    std::vector<std::uint8_t> content{};

    [[nodiscard]] bool operator==(Section const &) const = default;
};

// https://webassembly.github.io/spec/core/binary/types.html#types
enum class ValueType : std::uint8_t {
    // https://webassembly.github.io/spec/core/binary/types.html#number-types
    Int32 = 0x7f,
    Int64 = 0x7e,
    Float32 = 0x7d,
    Float64 = 0x7c,

    // https://webassembly.github.io/spec/core/binary/types.html#vector-types
    Vector128 = 0x7b,

    // https://webassembly.github.io/spec/core/binary/types.html#reference-types
    FunctionReference = 0x70,
    ExternReference = 0x6f,
};

// https://webassembly.github.io/spec/core/binary/types.html#result-types
using ResultType = std::vector<ValueType>;

// https://webassembly.github.io/spec/core/binary/types.html#function-types
struct FunctionType {
    ResultType parameters;
    ResultType results;

    [[nodiscard]] bool operator==(FunctionType const &) const = default;
};

// https://webassembly.github.io/spec/core/binary/modules.html#type-section
struct TypeSection {
    std::vector<FunctionType> types;

    [[nodiscard]] bool operator==(TypeSection const &) const = default;
};

// https://webassembly.github.io/spec/core/binary/modules.html#function-section
struct FunctionSection {
    std::vector<TypeIdx> type_indices;

    [[nodiscard]] bool operator==(FunctionSection const &) const = default;
};

// https://webassembly.github.io/spec/core/binary/modules.html#binary-export
struct Export {
    enum class Type { Function = 0, Table = 1, Memory = 2, Global = 3 };

    std::string name{};
    Type type{};
    std::uint32_t index{};

    [[nodiscard]] bool operator==(Export const &) const = default;
};

// https://webassembly.github.io/spec/core/binary/modules.html#binary-exportsec
struct ExportSection {
    std::vector<Export> exports{};

    [[nodiscard]] bool operator==(ExportSection const &) const = default;
};

// https://webassembly.github.io/spec/core/binary/modules.html#binary-codesec
struct CodeEntry {
    struct Local {
        std::uint32_t count{};
        ValueType type{};

        [[nodiscard]] bool operator==(Local const &) const = default;
    };

    std::vector<std::uint8_t> code{};
    std::vector<Local> locals{};

    [[nodiscard]] bool operator==(CodeEntry const &) const = default;
};
struct CodeSection {
    std::vector<CodeEntry> entries{};

    [[nodiscard]] bool operator==(CodeSection const &) const = default;
};

enum class ParseError {
    Unknown,
    UnexpectedEof,
    InvalidMagic,
    UnsupportedVersion,
    InvalidSectionId,
};

// https://webassembly.github.io/spec/core/syntax/modules.html
struct Module {
    static tl::expected<Module, ParseError> parse_from(std::istream &&is) { return parse_from(is); }
    static tl::expected<Module, ParseError> parse_from(std::istream &);

    std::vector<Section> sections{};

    std::optional<TypeSection> type_section() const;
    std::optional<FunctionSection> function_section() const;
    std::optional<ExportSection> export_section() const;
    std::optional<CodeSection> code_section() const;

    [[nodiscard]] bool operator==(Module const &) const = default;
};

} // namespace wasm

#endif
