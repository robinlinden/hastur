// SPDX-FileCopyrightText: 2025-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "html/parse.h"

#include <cstddef>
#include <cstdint>
#include <tuple>

extern "C" int LLVMFuzzerTestOneInput(std::uint8_t const *data, std::size_t size);

extern "C" int LLVMFuzzerTestOneInput(std::uint8_t const *data, std::size_t size) {
    std::ignore = html::parse({reinterpret_cast<char const *>(data), size},
            {
                    .include_comments = true,
            });
    std::ignore = html::parse({reinterpret_cast<char const *>(data), size},
            {
                    .scripting = true,
                    .include_comments = true,
            });
    return 0;
}
