// SPDX-FileCopyrightText: 2021 David Zero <zero-one@zer0-one.net>
// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "uri/uri.h"

#include "etest/etest.h"

using etest::expect;
using etest::expect_eq;
using uri::Uri;

int main() {
    etest::test("https: simple uri", [] {
        auto uri = *Uri::parse("https://example.com");
        Uri expected{
                .uri = "https://example.com",
                .scheme = "https",
                .authority = {.host = "example.com"},
        };

        expect(uri == expected);
    });

    etest::test("https: short uri", [] {
        auto uri = *Uri::parse("https://gr.ht");
        Uri expected{
                .uri = "https://gr.ht",
                .scheme = "https",
                .authority = {.host = "gr.ht"},
        };

        expect(uri == expected);
    });

    etest::test("https: user, pass, port, path, query", [] {
        auto https_uri =
                *Uri::parse("https://zero-one:muh_password@example-domain.net:8080/muh/long/path.html?foo=bar");

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
        auto https_uri = *Uri::parse("https://zero-one:muh_password@example-domain.net/muh/long/path.html?foo=bar");

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
        auto https_uri = *Uri::parse("https://zero-one@example-domain.net/muh/long/path.html?foo=bar");

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
        auto https_uri = *Uri::parse("https://example-domain.net/muh/long/path.html?foo=bar");

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
        auto https_uri = *Uri::parse("https://example-domain.net/muh/long/path.html#About");

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
        auto mailto_uri = *Uri::parse("mailto:example@example.net");

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
        auto tel_uri = *Uri::parse("tel:+1-830-476-5664");

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
        auto uri = *Uri::parse("hello/there.html");
        expect_eq(uri, Uri{.uri = "hello/there.html", .path = "hello/there.html"});
    });

    etest::test("absolute, no host", [] {
        auto uri = *Uri::parse("/hello/there.html");
        expect_eq(uri, Uri{.uri = "/hello/there.html", .path = "/hello/there.html"});
    });

    etest::test("scheme-relative", [] {
        auto uri = *Uri::parse("//example.com/hello/there.html");
        expect_eq(uri,
                Uri{.uri = "//example.com/hello/there.html",
                        .authority = {.host = "example.com"},
                        .path = "/hello/there.html"});
    });

    // TODO(Zer0-One): Test for parsing failure.
    // etest::test("parse failure", [] {
    //     auto tel_uri = Uri::parse("");
    //     expect(!tel_uri.has_value());
    // });

    return etest::run_all_tests();
}
