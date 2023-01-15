// SPDX-FileCopyrightText: 2023 David Zero <zero-one@zer0-one.net>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "net/ip.h"

#include "etest/etest.h"

#include <iostream>
#include <string>

using etest::require;

int main() {
    etest::test("IPv4 serialization", [] {
        std::uint32_t loopback = 2130706433;
        std::uint32_t global = 134744072;
        std::uint32_t nonroutable = 2886729729;

        std::cout << "Serialized IPv4 Loopback Address: " << net::ipv4_serialize(loopback) << "\n";
        std::cout << "Serialized IPv4 Globally-Routable Address: " << net::ipv4_serialize(global) << "\n";
        std::cout << "Serialized IPv4 RFC1918 Address: " << net::ipv4_serialize(nonroutable) << "\n";

        require(net::ipv4_serialize(loopback) == "127.0.0.1");
        require(net::ipv4_serialize(global) == "8.8.8.8");
        require(net::ipv4_serialize(nonroutable) == "172.16.0.1");
    });

    etest::test("IPv6 serialization", [] {
        std::uint16_t loopback[] = {0, 0, 0, 0, 0, 0, 0, 1};
        std::uint16_t global[] = {0x2001, 0xdb8, 0x85a3, 0, 0, 0x8a2e, 0x370, 0x7334};

        std::cout << "Serialized IPv6 Loopback Address: " << net::ipv6_serialize(loopback) << "\n";
        std::cout << "Serialized IPv6 Globally-Routable Address: " << net::ipv6_serialize(global) << "\n";

        require(net::ipv6_serialize(loopback) == "::1");
        require(net::ipv6_serialize(global) == "2001:db8:85a3::8a2e:370:7334");
    });

    return etest::run_all_tests();
}
