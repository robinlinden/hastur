// SPDX-FileCopyrightText: 2024 David Zero <zero-one@zer0-one.net>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef WASM_VALIDATION_H_
#define WASM_VALIDATION_H_

#include "wasm.h"

#include <tl/expected.hpp>

#include <cstdint>
#include <string_view>

namespace wasm::validation {

enum class ValidationError : std::uint8_t {
    BlockTypeInvalid,
    CodeSectionUndefined,
    ControlStackEmpty,
    FuncTypeInvalid,
    FunctionSectionUndefined,
    FuncUndefinedCode,
    LabelInvalid,
    LocalUndefined,
    MemoryBadAlignment,
    MemoryEmpty,
    MemorySectionUndefined,
    TypeSectionUndefined,
    UnknownInstruction,
    ValueStackHeightMismatch,
    ValueStackUnderflow,
    ValueStackUnexpected,
};

std::string_view to_string(ValidationError);

tl::expected<void, ValidationError> validate(Module const &);

} // namespace wasm::validation

#endif
