// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef ARCHIVE_ZLIB_H_
#define ARCHIVE_ZLIB_H_

#include <cstdint>
#include <expected>
#include <string>
#include <string_view>

namespace archive {

struct ZlibError {
    std::string message{};
    int code{};
};

enum class ZlibMode : std::uint8_t {
    Zlib,
    Gzip,
};

std::expected<std::string, ZlibError> zlib_decode(std::string_view, ZlibMode);

} // namespace archive

#endif
