// SPDX-FileCopyrightText: 2021 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "net/socket.h"

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

std::string read_impl(auto &socket) {
    std::string data;
    asio::error_code ec;
    asio::read(socket, asio::dynamic_buffer(data), ec);
    return data;
}

} // namespace

bool Socket::connect(std::string_view host, std::string_view service) {
    return connect_impl(resolver_, socket_, host, service);
}

std::size_t Socket::write(std::string_view data) {
    return write_impl(socket_, data);
}

std::string Socket::read() {
    return read_impl(socket_);
}

bool SecureSocket::connect(std::string_view host, std::string_view service) {
    if (connect_impl(resolver_, socket_.next_layer(), host, service)) {
        socket_.handshake(asio::ssl::stream_base::handshake_type::client);
        return true;
    }
    return false;
}

std::size_t SecureSocket::write(std::string_view data) {
    return write_impl(socket_, data);
}

std::string SecureSocket::read() {
    return read_impl(socket_);
}

} // namespace net
