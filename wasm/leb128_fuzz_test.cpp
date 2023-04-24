// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "wasm/leb128.h"

#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>
#include <tuple>

extern "C" int LLVMFuzzerTestOneInput(uint8_t const *data, size_t size); // NOLINT

extern "C" int LLVMFuzzerTestOneInput(uint8_t const *data, size_t size) {
    std::stringstream ss;

    ss = std::stringstream{std::string{reinterpret_cast<char const *>(data), size}};
    std::ignore = wasm::Leb128<int8_t>::decode_from(ss);
    ss = std::stringstream{std::string{reinterpret_cast<char const *>(data), size}};
    std::ignore = wasm::Leb128<int16_t>::decode_from(ss);
    ss = std::stringstream{std::string{reinterpret_cast<char const *>(data), size}};
    std::ignore = wasm::Leb128<int32_t>::decode_from(ss);
    ss = std::stringstream{std::string{reinterpret_cast<char const *>(data), size}};
    std::ignore = wasm::Leb128<int64_t>::decode_from(ss);

    ss = std::stringstream{std::string{reinterpret_cast<char const *>(data), size}};
    std::ignore = wasm::Leb128<uint8_t>::decode_from(ss);
    ss = std::stringstream{std::string{reinterpret_cast<char const *>(data), size}};
    std::ignore = wasm::Leb128<uint16_t>::decode_from(ss);
    ss = std::stringstream{std::string{reinterpret_cast<char const *>(data), size}};
    std::ignore = wasm::Leb128<uint32_t>::decode_from(ss);
    ss = std::stringstream{std::string{reinterpret_cast<char const *>(data), size}};
    std::ignore = wasm::Leb128<uint64_t>::decode_from(ss);

    return 0;
}
