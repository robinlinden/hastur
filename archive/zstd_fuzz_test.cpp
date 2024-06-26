// SPDX-FileCopyrightText: 2024 David Zero <zero-one@zer0-one.net>
// SPDX-FileCopyrightText: 2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "archive/zstd.h"

#include <cstddef>
#include <span>
#include <stddef.h> // NOLINT
#include <stdint.h> // NOLINT

extern "C" int LLVMFuzzerTestOneInput(uint8_t const *data, size_t size); // NOLINT

extern "C" int LLVMFuzzerTestOneInput(uint8_t const *data, size_t size) {
    std::ignore = archive::zstd_decode({reinterpret_cast<std::byte const *>(data), size});
    return 0;
}
