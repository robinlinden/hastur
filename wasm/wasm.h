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

namespace wasm {

// https://webassembly.github.io/spec/core/bikeshed/#sections
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

// https://webassembly.github.io/spec/core/bikeshed/#binary-export
struct Export {
    enum class Type { Function, Table, Memory, Global };

    std::string name{};
    Type type{};
    std::uint32_t index{};

    [[nodiscard]] bool operator==(Export const &) const = default;
};

// https://webassembly.github.io/spec/core/bikeshed/#export-section
struct ExportSection {
    std::vector<Export> exports{};

    [[nodiscard]] bool operator==(ExportSection const &) const = default;
};

// https://webassembly.github.io/spec/core/bikeshed/#modules
struct Module {
    static std::optional<Module> parse_from(std::istream &&is) { return parse_from(is); }
    static std::optional<Module> parse_from(std::istream &);

    std::vector<Section> sections{};

    std::optional<ExportSection> export_section() const;

    [[nodiscard]] bool operator==(Module const &) const = default;
};

} // namespace wasm

#endif
