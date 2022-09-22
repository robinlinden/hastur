// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "uri/uri.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <tuple>

extern "C" int LLVMFuzzerTestOneInput(uint8_t const *data, size_t size);

extern "C" int LLVMFuzzerTestOneInput(uint8_t const *data, size_t size) {
    std::ignore = uri::Uri::parse(std::string{reinterpret_cast<char const *>(data), size}).value();
    return 0;
}
