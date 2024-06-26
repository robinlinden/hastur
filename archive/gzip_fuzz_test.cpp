// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "archive/zlib.h"

#include <cstddef>
#include <stddef.h> // NOLINT
#include <stdint.h> // NOLINT
#include <tuple>

extern "C" int LLVMFuzzerTestOneInput(uint8_t const *data, size_t size); // NOLINT

extern "C" int LLVMFuzzerTestOneInput(uint8_t const *data, size_t size) {
    std::ignore = archive::zlib_decode({reinterpret_cast<std::byte const *>(data), size}, archive::ZlibMode::Gzip);
    return 0;
}
