// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "protocol/http.h"

#include "net/test/fake_socket.h"

#include <cstddef>
#include <cstdint>
#include <tuple>

extern "C" int LLVMFuzzerTestOneInput(std::uint8_t const *data, std::size_t size);

extern "C" int LLVMFuzzerTestOneInput(std::uint8_t const *data, std::size_t size) {
    net::FakeSocket socket{.read_data{reinterpret_cast<char const *>(data), size}};
    std::ignore = protocol::Http::get(socket, {}, std::nullopt);
    return 0;
}
