// SPDX-FileCopyrightText: 2021 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "net/socket.h"

#include <asio.hpp>
#include <asio/ssl.hpp>

namespace net {
namespace {

bool connect_impl(asio::ip::tcp::resolver &resolver,
        asio::ip::tcp::socket &socket,
        std::string_view host,
        std::string_view service) {
    asio::error_code ec;
    auto endpoints = resolver.resolve(host, service, ec);
    asio::connect(socket, endpoints, ec);
    return !ec;
}

std::size_t write_impl(auto &socket, std::string_view data) {
    asio::error_code ec;
    return asio::write(socket, asio::buffer(data), ec);
}

std::string read_all_impl(auto &socket) {
    std::string data;
    asio::error_code ec;
    asio::read(socket, asio::dynamic_buffer(data), ec);
    return data;
}

} // namespace

class Socket::Impl {
public:
    Impl() : resolver_(svc_), socket_(svc_) {}

    bool connect(std::string_view host, std::string_view service) {
        return connect_impl(resolver_, socket_, host, service);
    }
    std::size_t write(std::string_view data) { return write_impl(socket_, data); }

    std::string read_all() { return read_all_impl(socket_); }

private:
    asio::io_service svc_{};
    asio::ip::tcp::resolver resolver_;
    asio::ip::tcp::socket socket_;
};

Socket::Socket() : impl_(std::make_unique<Impl>()) {}

Socket::~Socket() = default;

Socket::Socket(Socket &&) = default;

Socket &Socket::operator=(Socket &&) = default;

bool Socket::connect(std::string_view host, std::string_view service) {
    return impl_->connect(host, service);
}

std::size_t Socket::write(std::string_view data) {
    return impl_->write(data);
}

std::string Socket::read_all() {
    return impl_->read_all();
}

class SecureSocket::Impl {
public:
    Impl() : resolver_(svc_), ctx_(asio::ssl::context::method::sslv23_client), socket_(svc_, ctx_) {}

    bool connect(std::string_view host, std::string_view service) {
        if (connect_impl(resolver_, socket_.next_layer(), host, service)) {
            socket_.handshake(asio::ssl::stream_base::handshake_type::client);
            return true;
        }
        return false;
    }

    std::size_t write(std::string_view data) { return write_impl(socket_, data); }

    std::string read_all() { return read_all_impl(socket_); }

private:
    asio::io_service svc_{};
    asio::ip::tcp::resolver resolver_;
    asio::ssl::context ctx_;
    asio::ssl::stream<asio::ip::tcp::socket> socket_;
};

SecureSocket::SecureSocket() : impl_(std::make_unique<Impl>()) {}

SecureSocket::~SecureSocket() = default;

SecureSocket::SecureSocket(SecureSocket &&) = default;

SecureSocket &SecureSocket::operator=(SecureSocket &&) = default;

bool SecureSocket::connect(std::string_view host, std::string_view service) {
    return impl_->connect(host, service);
}

std::size_t SecureSocket::write(std::string_view data) {
    return impl_->write(data);
}

std::string SecureSocket::read_all() {
    return impl_->read_all();
}

} // namespace net
