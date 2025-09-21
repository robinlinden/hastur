// SPDX-FileCopyrightText: 2023-2025 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2023 David Zero <zero-one@zer0-one.net>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "url/ip_serialization.h"

#include "etest/etest2.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <string>

int main() {
    etest::Suite es;

    es.add_test("IPv4 serialization", [](etest::IActions &a) {
        std::uint32_t loopback = 2130706433;
        std::uint32_t global = 134744072;
        std::uint32_t nonroutable = 2886729729;

        std::cout << "Serialized IPv4 Loopback Address: " << url::ipv4_serialize(loopback) << "\n";
        std::cout << "Serialized IPv4 Globally-Routable Address: " << url::ipv4_serialize(global) << "\n";
        std::cout << "Serialized IPv4 RFC1918 Address: " << url::ipv4_serialize(nonroutable) << "\n";

        a.expect(url::ipv4_serialize(loopback) == "127.0.0.1");
        a.expect(url::ipv4_serialize(global) == "8.8.8.8");
        a.expect(url::ipv4_serialize(nonroutable) == "172.16.0.1");
    });

    es.add_test("IPv6 serialization", [](etest::IActions &a) {
        std::array<std::uint16_t, 8> const loopback{0, 0, 0, 0, 0, 0, 0, 1};
        std::array<std::uint16_t, 8> global{0x2001, 0xdb8, 0x85a3, 0, 0, 0x8a2e, 0x370, 0x7334};

        std::cout << "Serialized IPv6 Loopback Address: " << url::ipv6_serialize(loopback) << "\n";
        std::cout << "Serialized IPv6 Globally-Routable Address: " << url::ipv6_serialize(global) << "\n";

        a.expect(url::ipv6_serialize(loopback) == "::1");
        a.expect(url::ipv6_serialize(global) == "2001:db8:85a3::8a2e:370:7334");
    });

    return es.run();
}
