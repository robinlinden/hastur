// SPDX-FileCopyrightText: 2022-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "css/parser.h"

#include <spdlog/common.h>
#include <spdlog/spdlog.h>

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <tuple>

extern "C" int LLVMFuzzerTestOneInput(std::uint8_t const *data, std::size_t size);

extern "C" int LLVMFuzzerTestOneInput(std::uint8_t const *data, std::size_t size) {
    // TODO(robinlinden): Nicer error handling and reporting.
    spdlog::set_level(spdlog::level::off);
    std::ignore = css::parse(std::string_view{reinterpret_cast<char const *>(data), size});
    return 0;
}
