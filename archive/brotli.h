// SPDX-FileCopyrightText: 2024 David Zero <zero-one@zer0-one.net>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef ARCHIVE_BROTLI_H_
#define ARCHIVE_BROTLI_H_

#include <tl/expected.hpp>

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace archive {

enum class BrotliError : std::uint8_t {
    InputEmpty,
};

std::string_view to_string(BrotliError);

tl::expected<std::vector<std::uint8_t>, BrotliError> brotli_decode(std::span<std::uint8_t const>);

} // namespace archive

#endif
