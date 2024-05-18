// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "protocol/http.h"

#include "net/test/fake_socket.h"

#include <cstddef>
#include <stddef.h> // NOLINT
#include <stdint.h> // NOLINT
#include <tuple>

extern "C" int LLVMFuzzerTestOneInput(uint8_t const *data, size_t size); // NOLINT

extern "C" int LLVMFuzzerTestOneInput(uint8_t const *data, size_t size) {
    net::FakeSocket socket{.read_data{reinterpret_cast<char const *>(data), size}};
    std::ignore = protocol::Http::get(socket, {}, std::nullopt);
    return 0;
}
