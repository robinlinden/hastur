// SPDX-FileCopyrightText: 2021 David Zero <zero-one@zer0-one.net>
// SPDX-FileCopyrightText: 2022-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "uri/uri.h"

#include "etest/etest2.h"

#include <optional>
#include <string>

using uri::Uri;

int main() {
    etest::Suite s;

    s.add_test("https: simple uri", [](etest::IActions &a) {
        auto uri = Uri::parse("https://example.com");
        Uri expected{
                .uri = "https://example.com",
                .scheme = "https",
                .authority = {.host = "example.com"},
                .path = "/",
        };

        a.expect(uri == expected);
    });

    s.add_test("https: short uri", [](etest::IActions &a) {
        auto uri = Uri::parse("https://gr.ht");
        Uri expected{
                .uri = "https://gr.ht",
                .scheme = "https",
                .authority = {.host = "gr.ht"},
                .path = "/",
        };

        a.expect(uri == expected);
    });

    s.add_test("empty uris don't parse as uris", [](etest::IActions &a) {
        a.expect_eq(Uri::parse(""), std::nullopt); //
    });

    s.add_test("large uris don't explode libstdc++", [](etest::IActions &a) {
        a.expect_eq(Uri::parse(std::string(1025, ':')), std::nullopt); //
    });

    s.add_test("large uris in are handled when base-uris are used", [](etest::IActions &a) {
        auto base = Uri::parse("https://example.com").value();
        a.expect_eq(Uri::parse(std::string(1020, '/'), base), std::nullopt);
        a.expect_eq(Uri::parse(std::string(1020, 'a'), base), std::nullopt);

        base = Uri::parse("https://example.com/foo/bar").value();
        a.expect_eq(Uri::parse(std::string(1020, 'a'), base), std::nullopt);
        a.expect_eq(Uri::parse("//" + std::string(1020, 'a'), base), std::nullopt);
    });

    s.add_test("https: user, pass, port, path, query", [](etest::IActions &a) {
        auto https_uri =
                Uri::parse("https://zero-one:muh_password@example-domain.net:8080/muh/long/path.html?foo=bar").value();

        a.expect(https_uri.scheme == "https");
        a.expect(https_uri.authority.user == "zero-one");
        a.expect(https_uri.authority.passwd == "muh_password");
        a.expect(https_uri.authority.host == "example-domain.net");
        a.expect(https_uri.authority.port == "8080");
        a.expect(https_uri.path == "/muh/long/path.html");
        a.expect(https_uri.query == "foo=bar");
        a.expect(https_uri.fragment.empty());
    });

    s.add_test("https: user, pass, path, query", [](etest::IActions &a) {
        auto https_uri =
                Uri::parse("https://zero-one:muh_password@example-domain.net/muh/long/path.html?foo=bar").value();

        a.expect(https_uri.scheme == "https");
        a.expect(https_uri.authority.user == "zero-one");
        a.expect(https_uri.authority.passwd == "muh_password");
        a.expect(https_uri.authority.host == "example-domain.net");
        a.expect(https_uri.authority.port.empty());
        a.expect(https_uri.path == "/muh/long/path.html");
        a.expect(https_uri.query == "foo=bar");
        a.expect(https_uri.fragment.empty());
    });

    s.add_test("https: user, path, query", [](etest::IActions &a) {
        auto https_uri = Uri::parse("https://zero-one@example-domain.net/muh/long/path.html?foo=bar").value();

        a.expect(https_uri.scheme == "https");
        a.expect(https_uri.authority.user == "zero-one");
        a.expect(https_uri.authority.passwd.empty());
        a.expect(https_uri.authority.host == "example-domain.net");
        a.expect(https_uri.authority.port.empty());
        a.expect(https_uri.path == "/muh/long/path.html");
        a.expect(https_uri.query == "foo=bar");
        a.expect(https_uri.fragment.empty());
    });

    s.add_test("https: path, query", [](etest::IActions &a) {
        auto https_uri = Uri::parse("https://example-domain.net/muh/long/path.html?foo=bar").value();

        a.expect(https_uri.scheme == "https");
        a.expect(https_uri.authority.user.empty());
        a.expect(https_uri.authority.passwd.empty());
        a.expect(https_uri.authority.host == "example-domain.net");
        a.expect(https_uri.authority.port.empty());
        a.expect(https_uri.path == "/muh/long/path.html");
        a.expect(https_uri.query == "foo=bar");
        a.expect(https_uri.fragment.empty());
    });

    s.add_test("https: path, fragment", [](etest::IActions &a) {
        auto https_uri = Uri::parse("https://example-domain.net/muh/long/path.html#About").value();

        a.expect(https_uri.scheme == "https");
        a.expect(https_uri.authority.user.empty());
        a.expect(https_uri.authority.passwd.empty());
        a.expect(https_uri.authority.host == "example-domain.net");
        a.expect(https_uri.authority.port.empty());
        a.expect(https_uri.path == "/muh/long/path.html");
        a.expect(https_uri.query.empty());
        a.expect(https_uri.fragment == "About");
    });

    s.add_test("mailto: path", [](etest::IActions &a) {
        auto mailto_uri = Uri::parse("mailto:example@example.net").value();

        a.expect(mailto_uri.scheme == "mailto");
        a.expect(mailto_uri.authority.user.empty());
        a.expect(mailto_uri.authority.passwd.empty());
        a.expect(mailto_uri.authority.host.empty());
        a.expect(mailto_uri.authority.port.empty());
        a.expect(mailto_uri.path == "example@example.net");
        a.expect(mailto_uri.query.empty());
        a.expect(mailto_uri.fragment.empty());
    });

    s.add_test("tel: path", [](etest::IActions &a) {
        auto tel_uri = Uri::parse("tel:+1-830-476-5664").value();

        a.expect(tel_uri.scheme == "tel");
        a.expect(tel_uri.authority.user.empty());
        a.expect(tel_uri.authority.passwd.empty());
        a.expect(tel_uri.authority.host.empty());
        a.expect(tel_uri.authority.port.empty());
        a.expect(tel_uri.path == "+1-830-476-5664");
        a.expect(tel_uri.query.empty());
        a.expect(tel_uri.fragment.empty());
    });

    s.add_test("relative, no host", [](etest::IActions &a) {
        auto uri = Uri::parse("hello/there.html").value();
        a.expect_eq(uri, Uri{.uri = "hello/there.html", .path = "hello/there.html"});
    });

    s.add_test("absolute, no host", [](etest::IActions &a) {
        auto uri = Uri::parse("/hello/there.html").value();
        a.expect_eq(uri, Uri{.uri = "/hello/there.html", .path = "/hello/there.html"});
    });

    s.add_test("scheme-relative", [](etest::IActions &a) {
        auto uri = Uri::parse("//example.com/hello/there.html").value();
        a.expect_eq(uri,
                Uri{.uri = "//example.com/hello/there.html",
                        .authority = {.host = "example.com"},
                        .path = "/hello/there.html"});
    });

    s.add_test("normalization, lowercasing scheme+host", [](etest::IActions &a) {
        auto actual = Uri::parse("HTTPS://EXAMPLE.COM/").value();
        Uri expected{
                .uri = "HTTPS://EXAMPLE.COM/",
                .scheme = "https",
                .authority = {.host = "example.com"},
                .path = "/",
        };

        a.expect_eq(actual, expected);
    });

    s.add_test("origin-relative completion", [](etest::IActions &a) {
        auto const base = uri::Uri::parse("hax://example.com").value();
        auto const completed = uri::Uri::parse("/test", base).value();
        a.expect_eq(completed, uri::Uri::parse("hax://example.com/test").value());
    });

    s.add_test("scheme-relative uri", [](etest::IActions &a) {
        auto const base = uri::Uri::parse("hax://example.com").value();
        auto const completed = uri::Uri::parse("//example2.com/test", base).value();
        a.expect_eq(completed, uri::Uri::parse("hax://example2.com/test").value());
    });

    s.add_test("path-relative uri", [](etest::IActions &a) {
        auto const base = uri::Uri::parse("hax://example.com").value();
        auto completed = uri::Uri::parse("test", base).value();
        a.expect_eq(completed, uri::Uri::parse("hax://example.com/test").value());

        completed = uri::Uri::parse("hello/", completed).value();
        a.expect_eq(completed, uri::Uri::parse("hax://example.com/hello/").value());

        completed = uri::Uri::parse("test", completed).value();
        a.expect_eq(completed, uri::Uri::parse("hax://example.com/hello/test").value());

        completed = uri::Uri::parse("goodbye", completed).value();
        a.expect_eq(completed, uri::Uri::parse("hax://example.com/hello/goodbye").value());
    });

    s.add_test("fragment completion", [](etest::IActions &a) {
        auto const base = uri::Uri::parse("hax://example.com").value();
        auto const completed = uri::Uri::parse("#test", base).value();
        a.expect_eq(completed, uri::Uri::parse("hax://example.com#test").value());
    });

    s.add_test("fragment completion, existing fragment", [](etest::IActions &a) {
        auto const base = uri::Uri::parse("hax://example.com#foo").value();
        auto const completed = uri::Uri::parse("#bar", base).value();
        a.expect_eq(completed, uri::Uri::parse("hax://example.com#bar").value());
    });

    return s.run();
}
