// SPDX-FileCopyrightText: 2024 David Zero <zero-one@zer0-one.net>
// SPDX-FileCopyrightText: 2024-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "archive/zstd.h"

#include <tl/expected.hpp>
#include <zstd.h>

#include <climits>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace archive {

std::string_view to_string(ZstdError err) {
    switch (err) {
        case ZstdError::DecodeEarlyTermination:
            return "Decoding terminated early; input is likely truncated";
        case ZstdError::DecompressionContext:
            return "Failed to create zstd decompression context";
        case ZstdError::InputEmpty:
            return "Input is empty";
        case ZstdError::MaximumOutputLengthExceeded:
            return "Output buffer exceeded maximum allowed length";
        case ZstdError::ZstdInternalError:
            return "Decode failure";
    }

    return "Unknown error";
}

tl::expected<std::vector<std::byte>, ZstdError> ZstdDecoder::decode(std::span<std::byte const> const input) const {
    if (input.empty()) {
        return tl::unexpected{ZstdError::InputEmpty};
    }

    std::unique_ptr<ZSTD_DCtx, decltype(&ZSTD_freeDCtx)> dctx(ZSTD_createDCtx(), &ZSTD_freeDCtx);

    if (dctx == nullptr) {
        return tl::unexpected{ZstdError::DecompressionContext};
    }

    std::size_t const chunk_size = ZSTD_DStreamOutSize();

    std::vector<std::byte> out;

    static_assert(CHAR_BIT == 8, "zstd requires 8-bit input");
    ZSTD_inBuffer in_buf = {input.data(), input.size_bytes(), 0};

    std::size_t count = 0;
    std::size_t last_ret = 0;
    std::size_t last_pos = 0;

    while (in_buf.pos < in_buf.size) {
        count++;

        out.resize(chunk_size * count);

        ZSTD_outBuffer out_buf = {out.data() + (chunk_size * (count - 1)), chunk_size, 0};

        std::size_t const ret = ZSTD_decompressStream(dctx.get(), &out_buf, &in_buf);

        if (ZSTD_isError(ret) != 0u) {
            return tl::unexpected{ZstdError::ZstdInternalError};
        }

        if (chunk_size * (count - 1) + out_buf.pos > max_output_length_) {
            return tl::unexpected{ZstdError::MaximumOutputLengthExceeded};
        }

        last_ret = ret;
        last_pos = out_buf.pos;
    }

    if (last_ret != 0) {
        return tl::unexpected{ZstdError::DecodeEarlyTermination};
    }

    std::size_t out_size = 0;

    if (count == 1) {
        out_size = last_pos;
    } else {
        out_size = last_pos == 0 ? chunk_size * count : (chunk_size * count) - (chunk_size - last_pos);
    }

    // Shrink buffer to match what we actually decoded
    out.resize(out_size);

    return out;
}

} // namespace archive
