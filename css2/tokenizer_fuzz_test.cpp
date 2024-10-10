// SPDX-FileCopyrightText: 2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "css2/tokenizer.h"

#include <cstddef>
#include <cstdint>
#include <string_view>

extern "C" int LLVMFuzzerTestOneInput(std::uint8_t const *data, std::size_t size);

extern "C" int LLVMFuzzerTestOneInput(std::uint8_t const *data, std::size_t size) {
    auto input = std::string_view{reinterpret_cast<char const *>(data), size};
    css2::Tokenizer{input,
            [](auto &&) {},
            [](auto) {
            }}
            .run();
    return 0;
}
