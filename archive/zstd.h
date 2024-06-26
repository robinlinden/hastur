// SPDX-FileCopyrightText: 2024 David Zero <zero-one@zer0-one.net>
// SPDX-FileCopyrightText: 2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef ARCHIVE_ZSTD_H_
#define ARCHIVE_ZSTD_H_

#include <tl/expected.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace archive {

enum class ZstdError : std::uint8_t {
    DecodeEarlyTermination,
    DecompressionContext,
    InputEmpty,
    MaximumOutputLengthExceeded,
    ZstdInternalError,
};

std::string_view to_string(ZstdError);

tl::expected<std::vector<std::byte>, ZstdError> zstd_decode(std::span<std::byte const>);

} // namespace archive

#endif
