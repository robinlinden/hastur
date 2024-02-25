// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "wasm/types.h"
#include "wasm/wasm.h"

#include <tl/expected.hpp>

#include <iosfwd>
#include <optional>

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
    InvalidExportSection,
    InvalidStartSection,
    InvalidCodeSection,
    UnhandledSection,
};

class ByteCodeParser {
public:
    static tl::expected<Module, ModuleParseError> parse_module(std::istream &);
    static tl::expected<Module, ModuleParseError> parse_module(std::istream &&is) { return parse_module(is); }

    // TODO(robinlinden): Make private once instructions are parsed eagerly.
    static std::optional<ValueType> parse_value_type(std::istream &);
};

} // namespace wasm
