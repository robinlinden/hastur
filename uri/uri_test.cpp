// SPDX-FileCopyrightText: 2021 David Zero <zero-one@zer0-one.net>
// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "uri/uri.h"

#include "etest/etest.h"

#include <cstdint>
#include <iostream>
#include <regex>

using etest::expect;
using etest::expect_eq;
using uri::Uri;

int main() {
    etest::test("https: simple uri", [] {
        auto uri = Uri::parse("https://example.com");
        Uri expected{
                .uri = "https://example.com",
                .scheme = "https",
                .authority = {.host = "example.com"},
                .path = "/",
        };

        expect(uri == expected);
    });

    etest::test("https: short uri", [] {
        auto uri = Uri::parse("https://gr.ht");
        Uri expected{
                .uri = "https://gr.ht",
                .scheme = "https",
                .authority = {.host = "gr.ht"},
                .path = "/",
        };

        expect(uri == expected);
    });

    etest::test("https: user, pass, port, path, query", [] {
        auto https_uri = Uri::parse("https://zero-one:muh_password@example-domain.net:8080/muh/long/path.html?foo=bar");

        expect(https_uri.scheme == "https");
        expect(https_uri.authority.user == "zero-one");
        expect(https_uri.authority.passwd == "muh_password");
        expect(https_uri.authority.host == "example-domain.net");
        expect(https_uri.authority.port == "8080");
        expect(https_uri.path == "/muh/long/path.html");
        expect(https_uri.query == "foo=bar");
        expect(https_uri.fragment.empty());
    });

    etest::test("https: user, pass, path, query", [] {
        auto https_uri = Uri::parse("https://zero-one:muh_password@example-domain.net/muh/long/path.html?foo=bar");

        expect(https_uri.scheme == "https");
        expect(https_uri.authority.user == "zero-one");
        expect(https_uri.authority.passwd == "muh_password");
        expect(https_uri.authority.host == "example-domain.net");
        expect(https_uri.authority.port.empty());
        expect(https_uri.path == "/muh/long/path.html");
        expect(https_uri.query == "foo=bar");
        expect(https_uri.fragment.empty());
    });

    etest::test("https: user, path, query", [] {
        auto https_uri = Uri::parse("https://zero-one@example-domain.net/muh/long/path.html?foo=bar");

        expect(https_uri.scheme == "https");
        expect(https_uri.authority.user == "zero-one");
        expect(https_uri.authority.passwd.empty());
        expect(https_uri.authority.host == "example-domain.net");
        expect(https_uri.authority.port.empty());
        expect(https_uri.path == "/muh/long/path.html");
        expect(https_uri.query == "foo=bar");
        expect(https_uri.fragment.empty());
    });

    etest::test("https: path, query", [] {
        auto https_uri = Uri::parse("https://example-domain.net/muh/long/path.html?foo=bar");

        expect(https_uri.scheme == "https");
        expect(https_uri.authority.user.empty());
        expect(https_uri.authority.passwd.empty());
        expect(https_uri.authority.host == "example-domain.net");
        expect(https_uri.authority.port.empty());
        expect(https_uri.path == "/muh/long/path.html");
        expect(https_uri.query == "foo=bar");
        expect(https_uri.fragment.empty());
    });

    etest::test("https: path, fragment", [] {
        auto https_uri = Uri::parse("https://example-domain.net/muh/long/path.html#About");

        expect(https_uri.scheme == "https");
        expect(https_uri.authority.user.empty());
        expect(https_uri.authority.passwd.empty());
        expect(https_uri.authority.host == "example-domain.net");
        expect(https_uri.authority.port.empty());
        expect(https_uri.path == "/muh/long/path.html");
        expect(https_uri.query.empty());
        expect(https_uri.fragment == "About");
    });

    etest::test("mailto: path", [] {
        auto mailto_uri = Uri::parse("mailto:example@example.net");

        expect(mailto_uri.scheme == "mailto");
        expect(mailto_uri.authority.user.empty());
        expect(mailto_uri.authority.passwd.empty());
        expect(mailto_uri.authority.host.empty());
        expect(mailto_uri.authority.port.empty());
        expect(mailto_uri.path == "example@example.net");
        expect(mailto_uri.query.empty());
        expect(mailto_uri.fragment.empty());
    });

    etest::test("tel: path", [] {
        auto tel_uri = Uri::parse("tel:+1-830-476-5664");

        expect(tel_uri.scheme == "tel");
        expect(tel_uri.authority.user.empty());
        expect(tel_uri.authority.passwd.empty());
        expect(tel_uri.authority.host.empty());
        expect(tel_uri.authority.port.empty());
        expect(tel_uri.path == "+1-830-476-5664");
        expect(tel_uri.query.empty());
        expect(tel_uri.fragment.empty());
    });

    etest::test("relative, no host", [] {
        auto uri = Uri::parse("hello/there.html");
        expect_eq(uri, Uri{.uri = "hello/there.html", .path = "hello/there.html"});
    });

    etest::test("absolute, no host", [] {
        auto uri = Uri::parse("/hello/there.html");
        expect_eq(uri, Uri{.uri = "/hello/there.html", .path = "/hello/there.html"});
    });

    etest::test("scheme-relative", [] {
        auto uri = Uri::parse("//example.com/hello/there.html");
        expect_eq(uri,
                Uri{.uri = "//example.com/hello/there.html",
                        .authority = {.host = "example.com"},
                        .path = "/hello/there.html"});
    });

    etest::test("normalization, lowercasing scheme+host", [] {
        auto actual = Uri::parse("HTTPS://EXAMPLE.COM/");
        Uri expected{
                .uri = "HTTPS://EXAMPLE.COM/",
                .scheme = "https",
                .authority = {.host = "example.com"},
                .path = "/",
        };

        expect_eq(actual, expected);
    });

    etest::test("origin-relative completion", [] {
        auto const base = uri::Uri::parse("hax://example.com");
        auto const completed = uri::Uri::parse("/test", base);
        expect_eq(completed, uri::Uri::parse("hax://example.com/test"));
    });

    etest::test("scheme-relative uri", [] {
        auto const base = uri::Uri::parse("hax://example.com");
        auto const completed = uri::Uri::parse("//example2.com/test", base);
        expect_eq(completed, uri::Uri::parse("hax://example2.com/test"));
    });

    etest::test("path-relative uri", [] {
        auto const base = uri::Uri::parse("hax://example.com");
        auto completed = uri::Uri::parse("test", base);
        expect_eq(completed, uri::Uri::parse("hax://example.com/test"));

        completed = uri::Uri::parse("hello", completed);
        expect_eq(completed, uri::Uri::parse("hax://example.com/test/hello"));
    });

    etest::test("blob URL generation", [] {
        std::string REGEX_UUID = "[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}";

        uri::Host h = {uri::DNS_DOMAIN, "example.com"};
        uri::Origin o = {"https", &h, (std::uint16_t)8080, std::nullopt, false};

        std::string blob = uri::blob_url_create(&o);
        std::cout << "Generated Blob URL: " << blob << std::endl;

        etest::require(std::regex_match(blob, std::regex("blob:https://example.com:8080/" + REGEX_UUID)));

        h = {uri::IP4_ADDR, "", 134744072};
        o = {"https", &h, (std::uint16_t)8080, std::nullopt, false};

        blob = uri::blob_url_create(&o);
        std::cout << "Generated Blob URL: " << blob << std::endl;

        etest::require(std::regex_match(blob, std::regex("blob:https://8.8.8.8:8080/" + REGEX_UUID)));

        h = {uri::IP6_ADDR, "", 0, {0x2001, 0xdb8, 0x85a3, 0, 0, 0x8a2e, 0x370, 0x7334}};
        o = {"https", &h, (std::uint16_t)8080, std::nullopt, false};

        blob = uri::blob_url_create(&o);
        std::cout << "Generated Blob URL: " << blob;

        etest::require(std::regex_match(
                blob, std::regex("blob:https://\\[2001:db8:85a3::8a2e:370:7334\\]:8080/" + REGEX_UUID)));
    });

    return etest::run_all_tests();
}
