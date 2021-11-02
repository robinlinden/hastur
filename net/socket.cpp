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

struct Socket::Impl {
    Impl() : resolver(svc), socket(svc) {}

    asio::io_service svc{};
    asio::ip::tcp::resolver resolver;
    asio::ip::tcp::socket socket;
};

Socket::Socket() : impl_(std::make_unique<Impl>()) {}

Socket::~Socket() = default;

Socket::Socket(Socket &&) = default;

Socket &Socket::operator=(Socket &&) = default;

bool Socket::connect(std::string_view host, std::string_view service) {
    return connect_impl(impl_->resolver, impl_->socket, host, service);
}

std::size_t Socket::write(std::string_view data) {
    return write_impl(impl_->socket, data);
}

std::string Socket::read_all() {
    return read_all_impl(impl_->socket);
}

struct SecureSocket::Impl {
    Impl() : resolver(svc), ctx(asio::ssl::context::method::sslv23_client), socket(svc, ctx) {}

    bool connect(std::string_view host, std::string_view service) {
        if (connect_impl(resolver, socket.next_layer(), host, service)) {
            socket.handshake(asio::ssl::stream_base::handshake_type::client);
            return true;
        }
        return false;
    }

    asio::io_service svc{};
    asio::ip::tcp::resolver resolver;
    asio::ssl::context ctx;
    asio::ssl::stream<asio::ip::tcp::socket> socket;
};

SecureSocket::SecureSocket() : impl_(std::make_unique<Impl>()) {}

SecureSocket::~SecureSocket() = default;

SecureSocket::SecureSocket(SecureSocket &&) = default;

SecureSocket &SecureSocket::operator=(SecureSocket &&) = default;

bool SecureSocket::connect(std::string_view host, std::string_view service) {
    return impl_->connect(host, service);
}

std::size_t SecureSocket::write(std::string_view data) {
    return write_impl(impl_->socket, data);
}

std::string SecureSocket::read_all() {
    return read_all_impl(impl_->socket);
}

} // namespace net
