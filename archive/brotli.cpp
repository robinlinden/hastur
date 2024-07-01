// SPDX-FileCopyrightText: 2024 David Zero <zero-one@zer0-one.net>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "archive/brotli.h"

#include "brotli/decode.h"
#include <tl/expected.hpp>

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace archive {

std::string_view to_string(BrotliError err) {
    switch (err) {
        case BrotliError::InputEmpty:
            return "Input is empty";
    }

    return "Unknown error";
}

tl::expected<std::vector<std::uint8_t>, BrotliError> brotli_decode(std::span<std::uint8_t const> const input) {
    if (input.empty()) {
        return tl::unexpected{BrotliError::InputEmpty};
    }

    return {};
}

} // namespace archive
