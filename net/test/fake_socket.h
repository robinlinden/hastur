// SPDX-FileCopyrightText: 2021-2022 Mikael Larsson <c.mikael.larsson@gmail.com>
// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef NET_TEST_FAKE_SOCKET_H_
#define NET_TEST_FAKE_SOCKET_H_

#include <cstddef>
#include <string>
#include <string_view>

namespace net {

struct FakeSocket {
    constexpr bool connect(std::string_view h, std::string_view s) {
        host = h;
        service = s;
        return connect_result;
    }

    constexpr std::size_t write(std::string_view data) {
        write_data = data;
        return write_data.size();
    }

    constexpr std::string read_all() const { return read_data; }

    constexpr std::string read_until(std::string_view d) {
        delimiter = d;
        std::string result;
        if (auto pos = read_data.find(d); pos != std::string::npos) {
            pos += d.size();
            result = read_data.substr(0, pos);
            read_data.erase(0, pos);
        }
        return result;
    }

    constexpr std::string read_bytes(std::size_t bytes) {
        std::string result = read_data.substr(0, bytes);
        read_data.erase(0, bytes);
        return result;
    }

    std::string host;
    std::string service;
    std::string write_data;
    std::string read_data;
    std::string delimiter;
    bool connect_result{true};
};

} // namespace net

#endif
