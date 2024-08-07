// SPDX-FileCopyrightText: 2024 David Zero <zero-one@zer0-one.net>
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
    DecoderState,
    InputCorrupt,
    InputEmpty,
    MaximumOutputLengthExceeded,
    BrotliInternalError,
};

std::string_view to_string(BrotliError);

tl::expected<std::vector<std::byte>, BrotliError> brotli_decode(std::span<std::byte const>);

} // namespace archive

#endif
