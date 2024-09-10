// SPDX-FileCopyrightText: 2023 David Zero <zero-one@zer0-one.net>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "url/url.h"

// NOLINTNEXTLINE(readability-identifier-naming)
extern "C" int LLVMFuzzerTestOneInput(std::uint8_t const *data, std::size_t size);

// NOLINTNEXTLINE(readability-identifier-naming)
extern "C" int LLVMFuzzerTestOneInput(std::uint8_t const *data, std::size_t size) {
    url::UrlParser p;
    std::optional<url::Url> url;

    url = p.parse({reinterpret_cast<char const *>(data), size});

    return 0;
}
