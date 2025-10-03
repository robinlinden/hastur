// SPDX-FileCopyrightText: 2021 Mikael Larsson <c.mikael.larsson@gmail.com>
// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef NET_SOCKET_H_
#define NET_SOCKET_H_

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>

namespace net {

class Socket {
public:
    Socket();
    ~Socket();

    Socket(Socket &&) noexcept;
    Socket &operator=(Socket &&) noexcept;

    [[nodiscard]] bool connect(std::string_view host, std::string_view service);
    bool disconnect();
    std::size_t write(std::string_view data);
    std::string read_all();
    std::string read_until(std::string_view delimiter);
    std::string read_bytes(std::size_t bytes);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

class SecureSocket {
public:
    SecureSocket();
    ~SecureSocket();

    SecureSocket(SecureSocket &&) noexcept;
    SecureSocket &operator=(SecureSocket &&) noexcept;

    [[nodiscard]] bool connect(std::string_view host, std::string_view service);
    bool disconnect();
    std::size_t write(std::string_view data);
    std::string read_all();
    std::string read_until(std::string_view delimiter);
    std::string read_bytes(std::size_t bytes);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace net

#endif
