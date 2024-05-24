// SPDX-FileCopyrightText: 2023 David Zero <zero-one@zer0-one.net>
// SPDX-FileCopyrightText: 2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "url/url.h"

#include <cstdint>
#include <cstring>
#include <stddef.h> // NOLINT
#include <stdint.h> // NOLINT
#include <string>
#include <tuple>

// NOLINTNEXTLINE(readability-identifier-naming)
extern "C" int LLVMFuzzerTestOneInput(uint8_t const *data, size_t size);

// NOLINTNEXTLINE(readability-identifier-naming)
extern "C" int LLVMFuzzerTestOneInput(uint8_t const *data, size_t size) {
    if (size < 1) {
        return 0;
    }

    bool has_base_url = data[0] % 2 == 0;
    data += 1;
    size -= 1;

    if (!has_base_url) {
        url::UrlParser p;
        std::ignore = p.parse({reinterpret_cast<char const *>(data), size});
        return 0;
    }

    if (size < 2) {
        return 0;
    }

    std::uint16_t base_url_length = 0;
    std::memcpy(&base_url_length, data, 2);
    data += 2;
    size -= 2;
    if (base_url_length > size) {
        return 0;
    }

    url::UrlParser base_parser;
    auto base_uri = base_parser.parse(std::string{reinterpret_cast<char const *>(data), base_url_length});
    data += base_url_length;
    size -= base_url_length;

    url::UrlParser parser;
    std::ignore = parser.parse(std::string{reinterpret_cast<char const *>(data), size}, base_uri);

    return 0;
}
