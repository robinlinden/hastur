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

    BrotliDecoderResult res = BROTLI_DECODER_RESULT_ERROR;

    std::vector<std::byte> intermediate_buf(kChunkSize);

    while (res != BROTLI_DECODER_RESULT_SUCCESS) {
        std::size_t avail_out = kChunkSize;
        auto *next_out = reinterpret_cast<std::uint8_t *>(intermediate_buf.data());

        res = BrotliDecoderDecompressStream(br_state.get(), &avail_in, &next_in, &avail_out, &next_out, nullptr);

        // Because we provide the whole input up-front, there's no reason we
        // would ever block on needing more input, except for corrupt data
        if (res == BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT) {
            return tl::unexpected{BrotliError::InputCorrupt};
        }

        if (res == BROTLI_DECODER_RESULT_ERROR) {
            auto e = BrotliDecoderGetErrorCode(br_state.get());
            switch (e) {
                // These are all the error codes related to bad input, indicated
                // by being prefixed w/ ERROR_FORMAT_.
                case BROTLI_DECODER_ERROR_FORMAT_EXUBERANT_NIBBLE:
                case BROTLI_DECODER_ERROR_FORMAT_RESERVED:
                case BROTLI_DECODER_ERROR_FORMAT_EXUBERANT_META_NIBBLE:
                case BROTLI_DECODER_ERROR_FORMAT_SIMPLE_HUFFMAN_ALPHABET:
                case BROTLI_DECODER_ERROR_FORMAT_SIMPLE_HUFFMAN_SAME:
                case BROTLI_DECODER_ERROR_FORMAT_CL_SPACE:
                case BROTLI_DECODER_ERROR_FORMAT_HUFFMAN_SPACE:
                case BROTLI_DECODER_ERROR_FORMAT_CONTEXT_MAP_REPEAT:
                case BROTLI_DECODER_ERROR_FORMAT_BLOCK_LENGTH_1:
                case BROTLI_DECODER_ERROR_FORMAT_BLOCK_LENGTH_2:
                case BROTLI_DECODER_ERROR_FORMAT_TRANSFORM:
                case BROTLI_DECODER_ERROR_FORMAT_DICTIONARY:
                case BROTLI_DECODER_ERROR_FORMAT_WINDOW_BITS:
                case BROTLI_DECODER_ERROR_FORMAT_PADDING_1:
                case BROTLI_DECODER_ERROR_FORMAT_PADDING_2:
                case BROTLI_DECODER_ERROR_FORMAT_DISTANCE:
                    return tl::unexpected{BrotliError::InputCorrupt};
                default:
                    return tl::unexpected{BrotliError::BrotliInternalError};
            }
        }

        if (out.size() + intermediate_buf.size() - avail_out >= max_output_length_) {
            return tl::unexpected{BrotliError::MaximumOutputLengthExceeded};
        }

        // TODO(zero-one): Replace with insert_range() when support is better. Requires P1206.
        out.insert(out.end(), intermediate_buf.begin(), intermediate_buf.end() - avail_out);
    }

    return out;
}

} // namespace archive
