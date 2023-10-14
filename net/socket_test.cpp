// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "net/socket.h"

#include "etest/etest.h"

#include <asio.hpp>
#include <asio/ssl.hpp>
#include <openssl/pem.h>
#include <openssl/x509.h>

#include <cstdint>
#include <cstdlib>
#include <future>
#include <iostream>
#include <string>
#include <thread>
#include <utility>

using etest::expect_eq;

namespace {

using PrivateKey = std::unique_ptr<EVP_PKEY, decltype([](EVP_PKEY *key) {
    if (key) {
        EVP_PKEY_free(key);
    }
})>;

using Rsa = std::unique_ptr<RSA, decltype([](RSA *rsa) {
    if (rsa) {
        RSA_free(rsa);
    }
})>;

PrivateKey generate_key() {
    PrivateKey pkey{EVP_PKEY_new()};
    if (!pkey) {
        return nullptr;
    }

    Rsa rsa{RSA_new()};
    if (!rsa) {
        return nullptr;
    }

    BIGNUM *e = BN_new();
    BN_set_word(e, RSA_F4);
    RSA_generate_key_ex(rsa.get(), 2048, e, nullptr);

    if (!EVP_PKEY_assign_RSA(pkey.get(), rsa.get())) {
        return nullptr;
    }

    rsa.release();
    return pkey;
}

using X509Certificate = std::unique_ptr<X509, decltype([](X509 *cert) {
    if (cert) {
        X509_free(cert);
    }
})>;

X509Certificate generate_x509(PrivateKey &pkey) {
    X509Certificate x509{X509_new()};
    if (!x509) {
        return nullptr;
    }

    ASN1_INTEGER_set(X509_get_serialNumber(x509.get()), 1);

    // 5 minutes should be plenty of time.
    X509_gmtime_adj(X509_get_notBefore(x509.get()), 0);
    X509_gmtime_adj(X509_getm_notAfter(x509.get()), 1000 * 5 * 60L);

    X509_set_pubkey(x509.get(), pkey.get());

    X509_NAME *name = X509_get_subject_name(x509.get());

    X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, reinterpret_cast<unsigned char const *>("SE"), -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, reinterpret_cast<unsigned char const *>("evilcorp"), -1, -1, 0);
    X509_NAME_add_entry_by_txt(
            name, "CN", MBSTRING_ASC, reinterpret_cast<unsigned char const *>("localhost"), -1, -1, 0);

    X509_set_issuer_name(x509.get(), name);

    if (!X509_sign(x509.get(), pkey.get(), EVP_sha1())) {
        return nullptr;
    }

    return x509;
}

enum class Type {
    Http,
    Https,
};

class Server {
public:
    explicit Server(std::string response, Type type = Type::Http) {
        std::promise<std::uint16_t> port_promise;
        port_future_ = port_promise.get_future();
        server_thread_ = std::thread{[payload = std::move(response), port = std::move(port_promise), type]() mutable {
            asio::io_context io_context;
            constexpr int kAnyPort = 0;
            asio::ip::tcp::acceptor a{io_context, asio::ip::tcp::endpoint{asio::ip::address_v4::loopback(), kAnyPort}};
            port.set_value(a.local_endpoint().port());
            if (type == Type::Http) {
                auto sock = a.accept();
                asio::write(sock, asio::buffer(payload, payload.size()));
            } else {
                asio::ssl::context ssl_context{asio::ssl::context::method::sslv23_server};

                auto key = generate_key();
                auto cert = generate_x509(key);
                auto *bio = BIO_new(BIO_s_mem());
                PEM_write_bio_X509(bio, cert.get());
                BUF_MEM *buf_mem;
                BIO_get_mem_ptr(bio, &buf_mem);
                asio::const_buffer cert_buffer{buf_mem->data, buf_mem->length};
                ssl_context.add_certificate_authority(cert_buffer);
                ssl_context.use_certificate(cert_buffer, asio::ssl::context::file_format::pem);
                // ssl_context.use_private_key(key.get(), asio::ssl::context::file_format::pem);

                asio::ssl::stream<asio::ip::tcp::socket> sock{io_context, ssl_context};
                a.accept(sock.next_layer());
                sock.handshake(asio::ssl::stream_base::handshake_type::server);
                asio::write(sock, asio::buffer(payload, payload.size()));
            }
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
    // etest::test("Socket::read_all", [] {
    //     auto server = Server{"hello!"};
    //     net::Socket sock;
    //     sock.connect("localhost", std::to_string(server.port()));

    //     expect_eq(sock.read_all(), "hello!");
    // });

    // etest::test("Socket::read_until", [] {
    //     auto server = Server{"beep\r\nbeep\r\nboop\r\n"};
    //     net::Socket sock;
    //     sock.connect("localhost", std::to_string(server.port()));

    //     expect_eq(sock.read_until("\r\n"), "beep\r\n");
    //     expect_eq(sock.read_until("\r\n"), "beep\r\n");
    //     expect_eq(sock.read_until("\r\n"), "boop\r\n");
    // });

    // etest::test("Socket::read_bytes", [] {
    //     auto server = Server{"123456789"};
    //     net::Socket sock;
    //     sock.connect("localhost", std::to_string(server.port()));

    //     expect_eq(sock.read_bytes(3), "123");
    //     expect_eq(sock.read_bytes(2), "45");
    //     expect_eq(sock.read_bytes(4), "6789");
    // });

    etest::test("SecureSocket::read_all", [] {
        auto server = Server{"hello!", Type::Https};
        net::SecureSocket sock;
        sock.connect("localhost", std::to_string(server.port()));

        expect_eq(sock.read_all(), "hello!");
    });

    // etest::test("SecureSocket::read_until", [] {
    //     auto port = start_server<Type::Https>("beep\r\nbeep\r\nboop\r\n");
    //     net::SecureSocket sock;
    //     sock.connect("localhost", std::to_string(port));

    //     expect_eq(sock.read_until("\r\n"), "beep\r\n");
    //     expect_eq(sock.read_until("\r\n"), "beep\r\n");
    //     expect_eq(sock.read_until("\r\n"), "boop\r\n");
    // });

    // etest::test("SecureSocket::read_bytes", [] {
    //     auto port = start_server<Type::Https>("123456789");
    //     net::SecureSocket sock;
    //     sock.connect("localhost", std::to_string(port));

    //     expect_eq(sock.read_bytes(3), "123");
    //     expect_eq(sock.read_bytes(2), "45");
    //     expect_eq(sock.read_bytes(4), "6789");
    // });

    return etest::run_all_tests();
}
