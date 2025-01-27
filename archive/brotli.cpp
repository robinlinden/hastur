// SPDX-FileCopyrightText: 2024 David Zero <zero-one@zer0-one.net>
// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "archive/brotli.h"

#include <brotli/decode.h>
#include <tl/expected.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace archive {

std::string_view to_string(BrotliError err) {
    switch (err) {
        case BrotliError::DecoderState:
            return "Failed to create brotli decoder state";
        case BrotliError::InputCorrupt:
            return "Input is corrupt or truncated";
        case BrotliError::InputEmpty:
            return "Input is empty";
        case BrotliError::MaximumOutputLengthExceeded:
            return "Output buffer exceeded maximum allowed length";
        case BrotliError::BrotliInternalError:
            return "Decode failure";
    }

    return "Unknown error";
}

tl::expected<std::vector<std::byte>, BrotliError> BrotliDecoder::decode(std::span<std::byte const> const input) const {
    if (input.empty()) {
        return tl::unexpected{BrotliError::InputEmpty};
    }

    std::unique_ptr<BrotliDecoderState, decltype(&BrotliDecoderDestroyInstance)> br_state(
            BrotliDecoderCreateInstance(nullptr, nullptr, nullptr), BrotliDecoderDestroyInstance);

    if (br_state == nullptr) {
        return tl::unexpected{BrotliError::DecoderState};
    }

    std::size_t constexpr kChunkSize = 131072; // Matches the zstd chunk size

    std::vector<std::byte> out;

    std::size_t avail_in = input.size();
    auto const *next_in = reinterpret_cast<std::uint8_t const *>(input.data());
    std::size_t total_out = 0;

    BrotliDecoderResult res = BROTLI_DECODER_RESULT_ERROR;

    std::vector<std::byte> intermediate_buf(kChunkSize);

    while (res != BROTLI_DECODER_RESULT_SUCCESS) {
        std::size_t avail_out = kChunkSize;
        auto *next_out = reinterpret_cast<std::uint8_t *>(intermediate_buf.data());

        res = BrotliDecoderDecompressStream(br_state.get(), &avail_in, &next_in, &avail_out, &next_out, &total_out);

        // Because we provide the whole input up-front, there's no reason we
        // would ever block on needing more input, except for corrupt data
        if (res == BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT) {
            return tl::unexpected{BrotliError::InputCorrupt};
        }

        if (res == BROTLI_DECODER_RESULT_ERROR) {
            // Brotli doesn't expose this in a sane way, so we use magic
            // numbers from the headers. -1 through -16 are errors related to
            // bad input.
            if (BrotliDecoderGetErrorCode(br_state.get()) <= -1 && BrotliDecoderGetErrorCode(br_state.get()) >= -16) {
                return tl::unexpected{BrotliError::InputCorrupt};
            }

            return tl::unexpected{BrotliError::BrotliInternalError};
        }

        if (out.size() + intermediate_buf.size() - avail_out >= max_output_length_) {
            return tl::unexpected{BrotliError::MaximumOutputLengthExceeded};
        }

        // TODO(zero-one): Replace with insert_range() when support is better
        out.insert(out.end(), intermediate_buf.begin(), intermediate_buf.end() - avail_out);
    }

    return out;
}

} // namespace archive
