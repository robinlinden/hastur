// SPDX-FileCopyrightText: 2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "wasm/validation.h"

#include "wasm/byte_code_parser.h"

#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>
#include <tuple>

extern "C" int LLVMFuzzerTestOneInput(std::uint8_t const *data, std::size_t size);

extern "C" int LLVMFuzzerTestOneInput(std::uint8_t const *data, std::size_t size) {
    auto module = wasm::ByteCodeParser::parse_module(
            std::stringstream{std::string{reinterpret_cast<char const *>(data), size}});
    if (!module) {
        return 0;
    }

    std::ignore = wasm::validation::validate(*module);
    return 0;
}
