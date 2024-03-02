// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "wasm/types.h"
#include "wasm/wasm.h"

#include <tl/expected.hpp>

#include <iosfwd>
#include <optional>
#include <string_view>

namespace wasm {

enum class ModuleParseError {
    UnexpectedEof,
    InvalidMagic,
    UnsupportedVersion,
    InvalidSectionId,
    InvalidSize,
    InvalidTypeSection,
    InvalidFunctionSection,
    InvalidTableSection,
    InvalidMemorySection,
    InvalidGlobalSection,
    InvalidExportSection,
    InvalidStartSection,
    InvalidCodeSection,
    UnhandledSection,
};

constexpr std::string_view to_string(ModuleParseError e) {
    switch (e) {
        case ModuleParseError::UnexpectedEof:
            return "Unexpected end of file";
        case ModuleParseError::InvalidMagic:
            return "Invalid magic number";
        case ModuleParseError::UnsupportedVersion:
            return "Unsupported version";
        case ModuleParseError::InvalidSectionId:
            return "Invalid section id";
        case ModuleParseError::InvalidSize:
            return "Invalid section size";
        case ModuleParseError::InvalidTypeSection:
            return "Invalid type section";
        case ModuleParseError::InvalidFunctionSection:
            return "Invalid function section";
        case ModuleParseError::InvalidTableSection:
            return "Invalid table section";
        case ModuleParseError::InvalidMemorySection:
            return "Invalid memory section";
        case ModuleParseError::InvalidGlobalSection:
            return "Invalid global section";
        case ModuleParseError::InvalidExportSection:
            return "Invalid export section";
        case ModuleParseError::InvalidStartSection:
            return "Invalid start section";
        case ModuleParseError::InvalidCodeSection:
            return "Invalid code section";
        case ModuleParseError::UnhandledSection:
            return "Unhandled section";
    }
    return "Unknown error";
}

class ByteCodeParser {
public:
    static tl::expected<Module, ModuleParseError> parse_module(std::istream &);
    static tl::expected<Module, ModuleParseError> parse_module(std::istream &&is) { return parse_module(is); }

    // TODO(robinlinden): Make private once instructions are parsed eagerly.
    static std::optional<ValueType> parse_value_type(std::istream &);
};

} // namespace wasm
