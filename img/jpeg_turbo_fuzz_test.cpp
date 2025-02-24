// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "img/jpeg_turbo.h"

#include <cstddef>
#include <cstdint>
#include <tuple>

extern "C" int LLVMFuzzerTestOneInput(std::uint8_t const *data, std::size_t size);

extern "C" int LLVMFuzzerTestOneInput(std::uint8_t const *data, std::size_t size) {
    std::ignore = img::JpegTurbo::from(std::span{reinterpret_cast<std::byte const *>(data), size});
    return 0;
}
