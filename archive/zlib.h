// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef ARCHIVE_ZLIB_H_
#define ARCHIVE_ZLIB_H_

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <vector>

namespace archive {

struct ZlibError {
    std::string message{};
    int code{};
};

enum class ZlibMode : std::uint8_t {
    Zlib,
    Gzip,
};

std::expected<std::vector<std::byte>, ZlibError> zlib_decode(std::span<std::byte const>, ZlibMode);

} // namespace archive

#endif
