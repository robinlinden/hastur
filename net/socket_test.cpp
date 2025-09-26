// SPDX-FileCopyrightText: 2023-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "net/socket.h"

#include "etest/etest2.h"

#include <asio/buffer.hpp>
#include <asio/error.hpp>
#include <asio/error_code.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/address_v4.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/write.hpp> // NOLINT: Needed for asio::write.

#include <array>
#include <cstdint>
#include <cstdlib>
#include <future>
#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <variant>

namespace {

class Server {
public:
    static Server writing_server(std::string response) { return Server{std::move(response)}; }
    static Server receiving_server(std::promise<std::string> received_data) { return Server{std::move(received_data)}; }

    ~Server() { server_thread_.join(); }

    std::uint16_t port() { return port_future_.get(); }

private:
    std::thread server_thread_;
    std::future<std::uint16_t> port_future_;

    explicit Server(std::variant<std::string, std::promise<std::string>> input) {
        std::promise<std::uint16_t> port_promise;
        port_future_ = port_promise.get_future();

        server_thread_ = std::thread{[payload = std::move(input), port = std::move(port_promise)]() mutable {
            asio::io_context io_context;
            constexpr int kAnyPort = 0;
            asio::ip::tcp::acceptor a{io_context, asio::ip::tcp::endpoint{asio::ip::address_v4::loopback(), kAnyPort}};
            port.set_value(a.local_endpoint().port());

            auto sock = a.accept();

            if (auto const *send_data = std::get_if<std::string>(&payload); send_data != nullptr) {
                // NOLINTNEXTLINE(misc-include-cleaner): Provided by <asio/write.hpp>.
                asio::write(sock, asio::buffer(*send_data, send_data->size()));
                return;
            }

            std::string received;
            std::array<char, 256> buf{};
            asio::error_code ec;
            while (true) {
                auto n = sock.read_some(asio::buffer(buf), ec);
                if (n > 0) {
                    std::cerr << "Server received " << n << " bytes\n";
                    received.append(buf.data(), n);
                }

                if (ec == asio::error::eof) {
                    std::cerr << "Server reached EOF\n";
                    break;
                }

                if (ec) {
                    std::cerr << "Error reading from socket: " << ec.message() << '\n';
                    std::abort();
                }
            }

            auto &receive_promise = std::get<std::promise<std::string>>(payload);
            receive_promise.set_value(std::move(received));
        }};
    }
};

} // namespace

int main() {
    etest::Suite s;

    s.add_test("Socket::read_all", [](etest::IActions &a) {
        auto server = Server::writing_server("hello!");
        net::Socket sock;
        a.require(sock.connect("localhost", std::to_string(server.port())));

        a.expect_eq(sock.read_all(), "hello!");
    });

    s.add_test("Socket::read_until", [](etest::IActions &a) {
        auto server = Server::writing_server("beep\r\nbeep\r\nboop\r\n");
        net::Socket sock;
        a.require(sock.connect("localhost", std::to_string(server.port())));

        a.expect_eq(sock.read_until("\r\n"), "beep\r\n");
        a.expect_eq(sock.read_until("\r\n"), "beep\r\n");
        a.expect_eq(sock.read_until("\r\n"), "boop\r\n");
    });

    s.add_test("Socket::read_bytes", [](etest::IActions &a) {
        auto server = Server::writing_server("123456789");
        net::Socket sock;
        a.require(sock.connect("localhost", std::to_string(server.port())));

        a.expect_eq(sock.read_bytes(3), "123");
        a.expect_eq(sock.read_bytes(2), "45");
        a.expect_eq(sock.read_bytes(4), "6789");
    });

    s.add_test("Socket::write", [](etest::IActions &a) {
        std::promise<std::string> received_data;
        auto future = received_data.get_future();

        net::Socket sock;
        auto server = Server::receiving_server(std::move(received_data));
        a.require(sock.connect("localhost", std::to_string(server.port())));

        a.expect_eq(sock.write("hello"), 5u);
        a.expect_eq(sock.write(" world"), 6u);
        a.expect(sock.disconnect());

        a.expect_eq(future.get(), "hello world");
    });

    return s.run();
}
