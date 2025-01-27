// SPDX-FileCopyrightText: 2024 David Zero <zero-one@zer0-one.net>
// SPDX-FileCopyrightText: 2024-2025 Robin Lindén <dev@robinlinden.eu>
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

class ZstdDecoder {
public:
    tl::expected<std::vector<std::byte>, ZstdError> decode(std::span<std::byte const>) const;

    void set_max_output_length(std::size_t length) { max_output_length_ = length; }

private:
    std::size_t max_output_length_ = std::size_t{1024} * 1024 * 1024;
};

inline tl::expected<std::vector<std::byte>, ZstdError> zstd_decode(std::span<std::byte const> input) {
    return ZstdDecoder{}.decode(input);
}

} // namespace archive

#endif
