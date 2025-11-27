// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "js/interpreter.h"

#include "js/parser.h"

#include <cstddef>
#include <cstdint>
#include <tuple>

extern "C" int LLVMFuzzerTestOneInput(std::uint8_t const *data, std::size_t size);

extern "C" int LLVMFuzzerTestOneInput(std::uint8_t const *data, std::size_t size) {
    auto ast = js::Parser::parse({reinterpret_cast<char const *>(data), size});
    if (!ast) {
        return 0;
    }

    auto interpreter = js::ast::Interpreter{};
    std::ignore = interpreter.execute(*ast);
    return 0;
}
