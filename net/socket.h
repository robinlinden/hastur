// SPDX-FileCopyrightText: 2021 Mikael Larsson <c.mikael.larsson@gmail.com>
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

    Socket(Socket &&);
    Socket &operator=(Socket &&);

    bool connect(std::string_view host, std::string_view service);
    std::size_t write(std::string_view data);
    std::string read_all();
    std::size_t read_until(std::string &data, std::string_view delimiter);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

class SecureSocket {
public:
    SecureSocket();
    ~SecureSocket();

    SecureSocket(SecureSocket &&);
    SecureSocket &operator=(SecureSocket &&);

    bool connect(std::string_view host, std::string_view service);
    std::size_t write(std::string_view data);
    std::string read_all();
    std::size_t read_until(std::string &data, std::string_view delimiter);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace net

#endif
