// SPDX-FileCopyrightText: 2021 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "net/socket.h"

#include <asio.hpp>
#include <asio/ssl.hpp>

#include <utility>

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
        asio::error_code ec;
        asio::read(socket, asio::dynamic_buffer(buffer), ec);
        return std::exchange(buffer, {});
    }

    std::string read_until(auto &socket, std::string_view delimiter) {
        asio::error_code ec;
        auto n = asio::read_until(socket, asio::dynamic_buffer(buffer), delimiter, ec);
        std::string result{};
        if (n > 0) {
            result = buffer.substr(0, n);
            buffer.erase(0, n);
        }
        return result;
    }

    std::string read_bytes(auto &socket, std::size_t bytes) {
        if (buffer.size() < bytes) {
            auto bytes_to_transfer = bytes - buffer.size();
            asio::error_code ec;
            asio::read(socket, asio::dynamic_buffer(buffer), asio::transfer_at_least(bytes_to_transfer), ec);
        }

        std::string result = buffer.substr(0, bytes);
        buffer.erase(0, bytes);
        return result;
    }
    std::string buffer{};
};

} // namespace

struct Socket::Impl : public BaseSocketImpl {
    asio::io_service svc{};
    asio::ip::tcp::resolver resolver{svc};
    asio::ip::tcp::socket socket{svc};
};

Socket::Socket() : impl_(std::make_unique<Impl>()) {}
Socket::~Socket() = default;
Socket::Socket(Socket &&) noexcept = default;
Socket &Socket::operator=(Socket &&) noexcept = default;

bool Socket::connect(std::string_view host, std::string_view service) {
    return impl_->connect(impl_->resolver, impl_->socket, host, service);
}

std::size_t Socket::write(std::string_view data) {
    return impl_->write(impl_->socket, data);
}

std::string Socket::read_all() {
    return impl_->read_all(impl_->socket);
}

std::string Socket::read_until(std::string_view delimiter) {
    return impl_->read_until(impl_->socket, delimiter);
}

std::string Socket::read_bytes(std::size_t bytes) {
    return impl_->read_bytes(impl_->socket, bytes);
}

struct SecureSocket::Impl : public BaseSocketImpl {
    bool connect(std::string_view host, std::string_view service) {
        if (BaseSocketImpl::connect(resolver, socket.next_layer(), host, service)) {
            socket.handshake(asio::ssl::stream_base::handshake_type::client);
            return true;
        }
        return false;
    }

    asio::io_service svc{};
    asio::ip::tcp::resolver resolver{svc};
    asio::ssl::context ctx{asio::ssl::context::method::sslv23_client};
    asio::ssl::stream<asio::ip::tcp::socket> socket{svc, ctx};
};

SecureSocket::SecureSocket() : impl_(std::make_unique<Impl>()) {}
SecureSocket::~SecureSocket() = default;
SecureSocket::SecureSocket(SecureSocket &&) noexcept = default;
SecureSocket &SecureSocket::operator=(SecureSocket &&) noexcept = default;

bool SecureSocket::connect(std::string_view host, std::string_view service) {
    return impl_->connect(host, service);
}

std::size_t SecureSocket::write(std::string_view data) {
    return impl_->write(impl_->socket, data);
}

std::string SecureSocket::read_all() {
    return impl_->read_all(impl_->socket);
}

std::string SecureSocket::read_until(std::string_view delimiter) {
    return impl_->read_until(impl_->socket, delimiter);
}

std::string SecureSocket::read_bytes(std::size_t bytes) {
    return impl_->read_bytes(impl_->socket, bytes);
}

} // namespace net
