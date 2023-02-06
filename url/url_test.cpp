// SPDX-FileCopyrightText: 2023 David Zero <zero-one@zer0-one.net>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "url/url.h"

#include "etest/etest.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <regex>

int main() {
    etest::test("blob URL generation", [] {
        std::string REGEX_UUID = "[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}";

        url::Host h = {url::HostType::DnsDomain, "example.com"};
        url::Origin o = {"https", h, std::uint16_t{8080}, std::nullopt, false};

        std::string blob = url::blob_url_create(o);
        std::cout << "Generated Blob URL: " << blob << std::endl;

        etest::expect(std::regex_match(blob, std::regex("blob:https://example.com:8080/" + REGEX_UUID)));

        h = url::Host{url::HostType::Ip4Addr, std::uint32_t{134744072}};
        o = {"https", h, std::uint16_t{8080}, std::nullopt, false};

        blob = url::blob_url_create(o);
        std::cout << "Generated Blob URL: " << blob << std::endl;

        etest::expect(std::regex_match(blob, std::regex("blob:https://8.8.8.8:8080/" + REGEX_UUID)));

        std::array<uint16_t, 8> v6 = {0x2001, 0xdb8, 0x85a3, 0, 0, 0x8a2e, 0x370, 0x7334};
        h = url::Host{url::HostType::Ip6Addr, v6};
        o = {"https", h, std::uint16_t{8080}, std::nullopt, false};

        blob = url::blob_url_create(o);
        std::cout << "Generated Blob URL: " << blob;

        etest::expect(std::regex_match(
                blob, std::regex("blob:https://\\[2001:db8:85a3::8a2e:370:7334\\]:8080/" + REGEX_UUID)));
    });

    return etest::run_all_tests();
}
