// SPDX-FileCopyrightText: 2021 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "net/socket.h"

#include <asio.hpp>
#include <asio/ssl.hpp>

namespace net {
namespace {

struct BaseSocketImpl {
    bool connect(asio::ip::tcp::resolver &resolver,
            asio::ip::tcp::socket &socket,
            std::string_view host,
            std::string_view service) {
        asio::error_code ec;
        auto endpoints = resolver.resolve(host, service, ec);
        asio::connect(socket, endpoints, ec);
        return !ec;
    }

    std::size_t write(auto &socket, std::string_view data) {
        asio::error_code ec;
        return asio::write(socket, asio::buffer(data), ec);
    }

    std::string read_all(auto &socket) {
        std::string data;
        asio::error_code ec;
        asio::read(socket, asio::dynamic_buffer(data), ec);
        return data;
    }

    std::size_t read_until(auto &socket, std::string &data, std::string_view delimiter) {
        asio::error_code ec;
        return asio::read_until(socket, asio::dynamic_buffer(data), delimiter, ec);
    }
};

} // namespace

struct Socket::Impl : public BaseSocketImpl {
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
    return impl_->connect(impl_->resolver, impl_->socket, host, service);
}

std::size_t Socket::write(std::string_view data) {
    return impl_->write(impl_->socket, data);
}

std::string Socket::read_all() {
    return impl_->read_all(impl_->socket);
}

std::size_t Socket::read_until(std::string &data, std::string_view delimiter) {
    return impl_->read_until(impl_->socket, data, delimiter);
}

struct SecureSocket::Impl : public BaseSocketImpl {
    Impl() : resolver(svc), ctx(asio::ssl::context::method::sslv23_client), socket(svc, ctx) {}

    bool connect(std::string_view host, std::string_view service) {
        if (BaseSocketImpl::connect(resolver, socket.next_layer(), host, service)) {
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
    return impl_->write(impl_->socket, data);
}

std::string SecureSocket::read_all() {
    return impl_->read_all(impl_->socket);
}

std::size_t SecureSocket::read_until(std::string &data, std::string_view delimiter) {
    return impl_->read_until(impl_->socket, data, delimiter);
}

} // namespace net
