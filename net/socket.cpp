// SPDX-FileCopyrightText: 2021 Mikael Larsson <c.mikael.larsson@gmail.com>
// SPDX-FileCopyrightText: 2023-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "net/socket.h"

#include <asio/buffer.hpp>
#include <asio/completion_condition.hpp>
#include <asio/connect.hpp> // NOLINT: Needed for asio::connect.
#include <asio/error_code.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/read.hpp>
#include <asio/read_until.hpp>
#include <asio/ssl/context.hpp>
#include <asio/ssl/stream.hpp>
#include <asio/ssl/stream_base.hpp>
// Provides us with SSL_set_tlsext_host_name, even if iwyu can't tell.
#include <openssl/ssl.h> // IWYU pragma: keep

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace net {
namespace {

struct BaseSocketImpl {
    [[nodiscard]] bool connect(asio::ip::tcp::resolver &resolver,
            asio::ip::tcp::socket &socket,
            std::string_view host,
            std::string_view service) {
        asio::error_code ec;
        auto endpoints = resolver.resolve(host, service, ec);
        // NOLINTNEXTLINE(misc-include-cleaner): Provided by <asio/connect.hpp>.
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
        std::string result;
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
    std::string buffer;
};

} // namespace

struct Socket::Impl : public BaseSocketImpl {
    bool disconnect() {
        asio::error_code ec;
        socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
        socket.close(ec);
        return !ec;
    }

    asio::io_context io_ctx;
    asio::ip::tcp::resolver resolver{io_ctx};
    asio::ip::tcp::socket socket{io_ctx};
};

Socket::Socket() : impl_(std::make_unique<Impl>()) {}
Socket::~Socket() = default;
Socket::Socket(Socket &&) noexcept = default;
Socket &Socket::operator=(Socket &&) noexcept = default;

bool Socket::connect(std::string_view host, std::string_view service) {
    return impl_->connect(impl_->resolver, impl_->socket, host, service);
}

bool Socket::disconnect() {
    return impl_->disconnect();
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
    // TODO(robinlinden): Better error propagation.
    bool connect(std::string_view host, std::string_view service) {
        if (BaseSocketImpl::connect(resolver, socket.next_layer(), host, service)) {
            asio::error_code ec;
            // Set SNI hostname. Many hosts reject the handshake if this isn't done.
            std::string null_terminated_host{host};
            SSL_set_tlsext_host_name(socket.native_handle(), null_terminated_host.c_str());
            socket.handshake(asio::ssl::stream_base::handshake_type::client, ec);
            return !ec;
        }
        return false;
    }

    bool disconnect() {
        asio::error_code ec;
        socket.shutdown(ec);
        socket.lowest_layer().close(ec);
        return !ec;
    }

    asio::io_context io_ctx;
    asio::ip::tcp::resolver resolver{io_ctx};
    asio::ssl::context ctx{asio::ssl::context::method::sslv23_client};
    asio::ssl::stream<asio::ip::tcp::socket> socket{io_ctx, ctx};
};

SecureSocket::SecureSocket() : impl_(std::make_unique<Impl>()) {}
SecureSocket::~SecureSocket() = default;
SecureSocket::SecureSocket(SecureSocket &&) noexcept = default;
SecureSocket &SecureSocket::operator=(SecureSocket &&) noexcept = default;

bool SecureSocket::connect(std::string_view host, std::string_view service) {
    return impl_->connect(host, service);
}

bool SecureSocket::disconnect() {
    return impl_->disconnect();
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
