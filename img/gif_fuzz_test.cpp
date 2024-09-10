// SPDX-FileCopyrightText: 2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "img/gif.h"

#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>
#include <tuple>

extern "C" int LLVMFuzzerTestOneInput(std::uint8_t const *data, std::size_t size); // NOLINT

extern "C" int LLVMFuzzerTestOneInput(std::uint8_t const *data, std::size_t size) {
    std::ignore = img::Gif::from( //
            std::stringstream{std::string{reinterpret_cast<char const *>(data), size}});
    return 0;
}
