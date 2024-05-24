// SPDX-FileCopyrightText: 2022-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "uri/uri.h"

#include <cstdint>
#include <cstring>
#include <stddef.h> // NOLINT
#include <stdint.h> // NOLINT
#include <string>
#include <tuple>

extern "C" int LLVMFuzzerTestOneInput(uint8_t const *data, size_t size); // NOLINT

extern "C" int LLVMFuzzerTestOneInput(uint8_t const *data, size_t size) {
    if (size < 1) {
        return 0;
    }

    bool has_base_uri = data[0] % 2 == 0;
    data += 1;
    size -= 1;

    if (!has_base_uri) {
        std::ignore = uri::Uri::parse(std::string{reinterpret_cast<char const *>(data), size});
        return 0;
    }

    if (size < 2) {
        return 0;
    }

    std::uint16_t base_uri_length = 0;
    std::memcpy(&base_uri_length, data, 2);
    data += 2;
    size -= 2;
    if (base_uri_length > size) {
        return 0;
    }

    auto base_uri = uri::Uri::parse(std::string{reinterpret_cast<char const *>(data), base_uri_length});
    data += base_uri_length;
    size -= base_uri_length;
    std::ignore = uri::Uri::parse(std::string{reinterpret_cast<char const *>(data), size}, base_uri);

    return 0;
}
