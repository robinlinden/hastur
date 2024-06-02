// SPDX-FileCopyrightText: 2024 David Zero <zero-one@zer0-one.net>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "archive/brotli.h"

#include <cstddef>
#include <span>
#include <stddef.h> // NOLINT
#include <stdint.h> // NOLINT

extern "C" int LLVMFuzzerTestOneInput(uint8_t const *data, size_t size); // NOLINT

extern "C" int LLVMFuzzerTestOneInput(uint8_t const *data, size_t size) {
    std::ignore = archive::brotli_decode({reinterpret_cast<std::byte const *>(data), size});
    return 0;
}
