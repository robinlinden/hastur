// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "css/parse.h"

#include <stddef.h> // NOLINT
#include <stdint.h> // NOLINT
#include <string_view>
#include <tuple>

extern "C" int LLVMFuzzerTestOneInput(uint8_t const *data, size_t size);

extern "C" int LLVMFuzzerTestOneInput(uint8_t const *data, size_t size) {
    std::ignore = css::parse(std::string_view{reinterpret_cast<char const *>(data), size});
    return 0;
}
