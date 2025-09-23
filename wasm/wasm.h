// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef WASM_WASM_H_
#define WASM_WASM_H_

#include "wasm/instructions.h"
#include "wasm/types.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace wasm {

struct CustomSection {
    std::string name;
    std::vector<std::uint8_t> data;

    [[nodiscard]] bool operator==(CustomSection const &) const = default;
};

// https://webassembly.github.io/spec/core/binary/types.html#binary-globaltype
struct GlobalType {
    enum class Mutability : std::uint8_t {
        Const,
        Var,
    };

    ValueType type{};
    Mutability mutability{};

    [[nodiscard]] bool operator==(GlobalType const &) const = default;
};

struct Global {
    GlobalType type{};
    std::vector<instructions::Instruction> init;

    [[nodiscard]] bool operator==(Global const &) const = default;
};

struct Import {
    using Description = std::variant<TypeIdx, TableType, MemType, GlobalType>;

    std::string module;
    std::string name;
    Description description;

    [[nodiscard]] bool operator==(Import const &) const = default;
};

// https://webassembly.github.io/spec/core/binary/modules.html#type-section
struct TypeSection {
    std::vector<FunctionType> types;

    [[nodiscard]] bool operator==(TypeSection const &) const = default;
};

// https://webassembly.github.io/spec/core/binary/modules.html#import-section
struct ImportSection {
    std::vector<Import> imports;

    [[nodiscard]] bool operator==(ImportSection const &) const = default;
};

// https://webassembly.github.io/spec/core/binary/modules.html#function-section
struct FunctionSection {
    std::vector<TypeIdx> type_indices;

    [[nodiscard]] bool operator==(FunctionSection const &) const = default;
};

// https://webassembly.github.io/spec/core/binary/modules.html#table-section
struct TableSection {
    std::vector<TableType> tables;

    [[nodiscard]] bool operator==(TableSection const &) const = default;
};

// https://webassembly.github.io/spec/core/binary/modules.html#memory-section
struct MemorySection {
    std::vector<MemType> memories;

    [[nodiscard]] bool operator==(MemorySection const &) const = default;
};

// https://webassembly.github.io/spec/core/binary/modules.html#binary-globalsec
struct GlobalSection {
    std::vector<Global> globals;

    [[nodiscard]] bool operator==(GlobalSection const &) const = default;
};

// https://webassembly.github.io/spec/core/binary/modules.html#binary-export
struct Export {
    enum class Type : std::uint8_t { Function = 0, Table = 1, Memory = 2, Global = 3 };

    std::string name;
    Type type{};
    std::uint32_t index{};

    [[nodiscard]] bool operator==(Export const &) const = default;
};

// https://webassembly.github.io/spec/core/binary/modules.html#binary-exportsec
struct ExportSection {
    std::vector<Export> exports;

    [[nodiscard]] bool operator==(ExportSection const &) const = default;
};

struct StartSection {
    FuncIdx start{};

    [[nodiscard]] bool operator==(StartSection const &) const = default;
};

// https://webassembly.github.io/spec/core/binary/modules.html#binary-codesec
struct CodeEntry {
    struct Local {
        std::uint32_t count{};
        ValueType type{};

        [[nodiscard]] bool operator==(Local const &) const = default;
    };

    std::vector<instructions::Instruction> code;
    std::vector<Local> locals;

    [[nodiscard]] bool operator==(CodeEntry const &) const = default;
};
struct CodeSection {
    std::vector<CodeEntry> entries;

    [[nodiscard]] bool operator==(CodeSection const &) const = default;
};

// https://webassembly.github.io/spec/core/binary/modules.html#data-section
struct DataSection {
    struct ActiveData {
        std::uint32_t memory_idx{};
        std::vector<instructions::Instruction> offset;
        std::vector<std::byte> data;

        [[nodiscard]] bool operator==(ActiveData const &) const = default;
    };

    struct PassiveData {
        std::vector<std::byte> data;

        [[nodiscard]] bool operator==(PassiveData const &) const = default;
    };

    using Data = std::variant<ActiveData, PassiveData>;
    std::vector<Data> data;

    [[nodiscard]] bool operator==(DataSection const &) const = default;
};

struct DataCountSection {
    std::uint32_t count{};

    [[nodiscard]] bool operator==(DataCountSection const &) const = default;
};

// https://webassembly.github.io/spec/core/syntax/modules.html
struct Module {
    std::vector<CustomSection> custom_sections;

    std::optional<TypeSection> type_section;
    std::optional<ImportSection> import_section;
    std::optional<FunctionSection> function_section;
    std::optional<TableSection> table_section;
    std::optional<MemorySection> memory_section;
    std::optional<GlobalSection> global_section;
    std::optional<ExportSection> export_section;
    std::optional<StartSection> start_section;
    // TODO(robinlinden): element_section
    std::optional<CodeSection> code_section;
    std::optional<DataSection> data_section;
    std::optional<DataCountSection> data_count_section;

    [[nodiscard]] bool operator==(Module const &) const = default;
};

} // namespace wasm

#endif
