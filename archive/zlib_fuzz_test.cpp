// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "archive/zlib.h"

#include <stddef.h> // NOLINT
#include <stdint.h> // NOLINT
#include <string_view>
#include <tuple>

extern "C" int LLVMFuzzerTestOneInput(uint8_t const *data, size_t size); // NOLINT

extern "C" int LLVMFuzzerTestOneInput(uint8_t const *data, size_t size) {
    std::string_view input{reinterpret_cast<char const *>(data), size};
    std::ignore = archive::zlib_decode(input, archive::ZlibMode::Zlib);
    return 0;
}
