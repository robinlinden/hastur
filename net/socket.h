// SPDX-FileCopyrightText: 2021 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef NET_SOCKET_H_
#define NET_SOCKET_H_

#include <asio.hpp>
#include <asio/ssl.hpp>

#include <cstddef>
#include <string>
#include <string_view>

namespace net {

class Socket {
public:
    Socket() : resolver_(svc_), socket_(svc_) {}

    bool connect(std::string_view host, std::string_view service);
    std::size_t write(std::string_view data);
    std::string read();

private:
    asio::io_service svc_{};
    asio::ip::tcp::resolver resolver_;
    asio::ip::tcp::socket socket_;
};

class SecureSocket {
public:
    SecureSocket() : resolver_(svc_), ctx_(asio::ssl::context::method::sslv23_client), socket_(svc_, ctx_) {}

    bool connect(std::string_view host, std::string_view service);
    std::size_t write(std::string_view data);
    std::string read();

private:
    asio::io_service svc_{};
    asio::ip::tcp::resolver resolver_;
    asio::ssl::context ctx_;
    asio::ssl::stream<asio::ip::tcp::socket> socket_;
};

} // namespace net

#endif
