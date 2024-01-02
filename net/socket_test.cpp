// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "net/socket.h"

#include "etest/etest2.h"

#include <asio/buffer.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/address_v4.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/write.hpp>

#include <cstdint>
#include <future>
#include <string>
#include <thread>
#include <utility>

namespace {

class Server {
public:
    explicit Server(std::string response) {
        std::promise<std::uint16_t> port_promise;
        port_future_ = port_promise.get_future();

        server_thread_ = std::thread{[payload = std::move(response), port = std::move(port_promise)]() mutable {
            asio::io_context io_context;
            constexpr int kAnyPort = 0;
            asio::ip::tcp::acceptor a{io_context, asio::ip::tcp::endpoint{asio::ip::address_v4::loopback(), kAnyPort}};
            port.set_value(a.local_endpoint().port());

            auto sock = a.accept();
            asio::write(sock, asio::buffer(payload, payload.size()));
        }};
    }

    ~Server() { server_thread_.join(); }

    std::uint16_t port() { return port_future_.get(); }

private:
    std::thread server_thread_{};
    std::future<std::uint16_t> port_future_{};
};

} // namespace

int main() {
    etest::Suite s;

    s.add_test("Socket::read_all", [](etest::IActions &a) {
        auto server = Server{"hello!"};
        net::Socket sock;
        sock.connect("localhost", std::to_string(server.port()));

        a.expect_eq(sock.read_all(), "hello!");
    });

    s.add_test("Socket::read_until", [](etest::IActions &a) {
        auto server = Server{"beep\r\nbeep\r\nboop\r\n"};
        net::Socket sock;
        sock.connect("localhost", std::to_string(server.port()));

        a.expect_eq(sock.read_until("\r\n"), "beep\r\n");
        a.expect_eq(sock.read_until("\r\n"), "beep\r\n");
        a.expect_eq(sock.read_until("\r\n"), "boop\r\n");
    });

    s.add_test("Socket::read_bytes", [](etest::IActions &a) {
        auto server = Server{"123456789"};
        net::Socket sock;
        sock.connect("localhost", std::to_string(server.port()));

        a.expect_eq(sock.read_bytes(3), "123");
        a.expect_eq(sock.read_bytes(2), "45");
        a.expect_eq(sock.read_bytes(4), "6789");
    });

    return s.run();
}
