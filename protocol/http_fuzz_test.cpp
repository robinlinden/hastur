// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "protocol/http.h"

#include <cstddef>
#include <stddef.h> // NOLINT
#include <stdint.h> // NOLINT
#include <string>
#include <string_view>
#include <tuple>

namespace {
struct FakeSocket {
    std::string read_data{};

    bool connect(std::string_view, std::string_view) { return true; }
    std::size_t write(std::string_view data) { return data.size(); }
    std::string read_all() const { return read_data; }

    std::string read_until(std::string_view d) {
        std::string result{};
        if (auto pos = read_data.find(d); pos != std::string::npos) {
            pos += d.size();
            result = read_data.substr(0, pos);
            read_data.erase(0, pos);
        }
        return result;
    }

    std::string read_bytes(std::size_t bytes) {
        std::string result = read_data.substr(0, bytes);
        read_data.erase(0, bytes);
        return result;
    }
};
} // namespace

extern "C" int LLVMFuzzerTestOneInput(uint8_t const *data, size_t size); // NOLINT

extern "C" int LLVMFuzzerTestOneInput(uint8_t const *data, size_t size) {
    FakeSocket socket{.read_data{reinterpret_cast<char const *>(data), size}};
    std::ignore = protocol::Http::get(socket, {}, std::nullopt);
    return 0;
}
