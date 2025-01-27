// SPDX-FileCopyrightText: 2024 David Zero <zero-one@zer0-one.net>
// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef ARCHIVE_BROTLI_H_
#define ARCHIVE_BROTLI_H_

#include <tl/expected.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace archive {

enum class BrotliError : std::uint8_t {
    BrotliInternalError,
    DecoderState,
    InputCorrupt,
    InputEmpty,
    MaximumOutputLengthExceeded,
};

std::string_view to_string(BrotliError);

class BrotliDecoder {
public:
    tl::expected<std::vector<std::byte>, BrotliError> decode(std::span<std::byte const>) const;

    void set_max_output_length(std::size_t length) { max_output_length_ = length; }

private:
    std::size_t max_output_length_ = std::size_t{1024} * 1024 * 1024;
};

inline tl::expected<std::vector<std::byte>, BrotliError> brotli_decode(std::span<std::byte const> input) {
    return BrotliDecoder{}.decode(input);
}

} // namespace archive

#endif
