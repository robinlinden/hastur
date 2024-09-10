// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "archive/zlib.h"

#include <cstddef>
#include <cstdint>
#include <tuple>

extern "C" int LLVMFuzzerTestOneInput(std::uint8_t const *data, std::size_t size); // NOLINT

extern "C" int LLVMFuzzerTestOneInput(std::uint8_t const *data, std::size_t size) {
    std::ignore = archive::zlib_decode({reinterpret_cast<std::byte const *>(data), size}, archive::ZlibMode::Zlib);
    return 0;
}
