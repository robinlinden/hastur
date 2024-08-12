// SPDX-FileCopyrightText: 2024 David Zero <zero-one@zer0-one.net>
// SPDX-FileCopyrightText: 2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "archive/zstd.h"

#include <zstd.h>

#include <climits>
#include <cstddef>
#include <cstdlib>
#include <expected>
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

std::expected<std::vector<std::byte>, ZstdError> zstd_decode(std::span<std::byte const> const input) {
    if (input.empty()) {
        return std::unexpected{ZstdError::InputEmpty};
    }

    std::unique_ptr<ZSTD_DCtx, decltype(&ZSTD_freeDCtx)> dctx(ZSTD_createDCtx(), &ZSTD_freeDCtx);

    if (dctx == nullptr) {
        return std::unexpected{ZstdError::DecompressionContext};
    }

    // Cap output buffer at 1GB. If we hit this, something fishy is probably
    // going on, and we should bail before we OOM.
    std::size_t constexpr kMaxOutSize = 1000000000;

    std::size_t const chunk_size = ZSTD_DStreamOutSize();

    std::vector<std::byte> out;

    static_assert(CHAR_BIT == 8, "zstd requires 8-bit input");
    ZSTD_inBuffer in_buf = {input.data(), input.size_bytes(), 0};

    std::size_t count = 0;
    std::size_t last_ret = 0;
    std::size_t last_pos = 0;

    while (in_buf.pos < in_buf.size) {
        count++;

        if ((chunk_size * count) > kMaxOutSize) {
            return std::unexpected{ZstdError::MaximumOutputLengthExceeded};
        }

        out.resize(chunk_size * count);

        ZSTD_outBuffer out_buf = {out.data() + (chunk_size * (count - 1)), chunk_size, 0};

        std::size_t const ret = ZSTD_decompressStream(dctx.get(), &out_buf, &in_buf);

        if (ZSTD_isError(ret) != 0u) {
            return std::unexpected{ZstdError::ZstdInternalError};
        }

        last_ret = ret;
        last_pos = out_buf.pos;
    }

    if (last_ret != 0) {
        return std::unexpected{ZstdError::DecodeEarlyTermination};
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
