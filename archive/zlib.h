// SPDX-FileCopyrightText: 2023-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef ARCHIVE_ZLIB_H_
#define ARCHIVE_ZLIB_H_

#include <tl/expected.hpp>

#include <cstddef>
#include <cstdint>
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

tl::expected<std::vector<std::byte>, ZlibError> zlib_decode(
        std::span<std::byte const>, ZlibMode, std::size_t max_output_length = std::size_t{1024} * 1024 * 1024);

} // namespace archive

#endif
