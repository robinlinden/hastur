// SPDX-FileCopyrightText: 2023 David Zero <zero-one@zer0-one.net>
// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "url/url.h"

#include "etest/etest.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <optional>
#include <regex>
#include <variant>

int main() {
    const url::Url base{"https",
            "",
            "",
            url::Host{url::HostType::DnsDomain, "example.com"},
            std::uint16_t{8080},
            std::vector<std::string>{"test", "index.php"}};

    etest::test("blob URL generation", [] {
        std::string regex_uuid = "[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}";

        url::Host h = {url::HostType::DnsDomain, "example.com"};
        url::Origin o = {"https", h, std::uint16_t{8080}, std::nullopt, false};

        std::string blob = url::blob_url_create(o);
        std::cout << std::endl << "Generated Blob URL: " << blob << std::endl;

        etest::expect(std::regex_match(blob, std::regex("blob:https://example.com:8080/" + regex_uuid)));

        h = url::Host{url::HostType::Ip4Addr, std::uint32_t{134744072}};
        o = {"https", h, std::uint16_t{8080}, std::nullopt, false};

        blob = url::blob_url_create(o);
        std::cout << "Generated Blob URL: " << blob << std::endl;

        etest::expect(std::regex_match(blob, std::regex("blob:https://8.8.8.8:8080/" + regex_uuid)));

        std::array<std::uint16_t, 8> v6 = {0x2001, 0xdb8, 0x85a3, 0, 0, 0x8a2e, 0x370, 0x7334};
        h = url::Host{url::HostType::Ip6Addr, v6};
        o = {"https", h, std::uint16_t{8080}, std::nullopt, false};

        blob = url::blob_url_create(o);
        std::cout << "Generated Blob URL: " << blob << std::endl;

        etest::expect(std::regex_match(
                blob, std::regex("blob:https://\\[2001:db8:85a3::8a2e:370:7334\\]:8080/" + regex_uuid)));
    });

    etest::test("URL parsing: port and path", [] {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("https://example.com:8080/index.html");

        etest::require(url.has_value());

        etest::expect_eq(url->scheme, "https");
        etest::expect_eq(std::get<0>(url->host->data), "example.com");
        etest::expect_eq(url->port.value(), 8080);
        etest::expect_eq(std::get<1>(url->path)[0], "index.html");
        etest::expect(!url->query.has_value());
        etest::expect(!url->fragment.has_value());

        etest::expect_eq(url->serialize(), "https://example.com:8080/index.html");
    });

    etest::test("URL parsing: 1 unicode char", [] {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("http://bücher.de");

        etest::require(url.has_value());

        etest::expect_eq(url->scheme, "http");
        etest::expect_eq(std::get<0>(url->host->data), "xn--bcher-kva.de");
        etest::expect(!url->port.has_value());
        etest::expect_eq(std::get<1>(url->path)[0], "");
        etest::expect(!url->query.has_value());
        etest::expect(!url->fragment.has_value());

        etest::expect_eq(url->serialize(), "http://xn--bcher-kva.de/");
    });

    etest::test("URL parsing: 1 unicode char with path", [] {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("https://√.com/i/itunes.gif");

        etest::require(url.has_value());

        etest::expect_eq(url->scheme, "https");
        etest::expect_eq(std::get<0>(url->host->data), "xn--19g.com");
        etest::expect(!url->port.has_value());
        etest::expect_eq(std::get<1>(url->path)[0], "i");
        etest::expect_eq(std::get<1>(url->path)[1], "itunes.gif");
        etest::expect(!url->query.has_value());
        etest::expect(!url->fragment.has_value());

        etest::expect_eq(url->serialize(), "https://xn--19g.com/i/itunes.gif");
    });

    etest::test("URL parsing: unicode path", [] {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("https://ar.wikipedia.org/wiki/نجيب_محفوظ");

        etest::require(url.has_value());

        etest::expect_eq(url->scheme, "https");
        etest::expect_eq(std::get<0>(url->host->data), "ar.wikipedia.org");
        etest::expect(!url->port.has_value());
        etest::expect_eq(std::get<1>(url->path)[0], "wiki");
        etest::expect_eq(std::get<1>(url->path)[1], "%D9%86%D8%AC%D9%8A%D8%A8_%D9%85%D8%AD%D9%81%D9%88%D8%B8");
        etest::expect(!url->query.has_value());
        etest::expect(!url->fragment.has_value());

        etest::expect_eq(url->serialize(),
                "https://ar.wikipedia.org/wiki/%D9%86%D8%AC%D9%8A%D8%A8_%D9%85%D8%AD%D9%81%D9%88%D8%B8");
    });

    etest::test("URL parsing: tel URI", [] {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("tel:+1-555-555-5555");

        etest::require(url.has_value());

        etest::expect_eq(url->scheme, "tel");
        etest::expect(!url->host.has_value());
        etest::expect(!url->port.has_value());
        etest::expect_eq(std::get<0>(url->path), "+1-555-555-5555");
        etest::expect(!url->query.has_value());
        etest::expect(!url->fragment.has_value());

        etest::expect_eq(url->serialize(), "tel:+1-555-555-5555");
    });

    etest::test("URL parsing: username and passwd in authority", [] {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("https://zero-one:testpass123@example.com/login.php");

        etest::require(url.has_value());

        etest::expect_eq(url->scheme, "https");
        etest::expect_eq(url->user, "zero-one");
        etest::expect_eq(url->passwd, "testpass123");
        etest::expect_eq(std::get<0>(url->host->data), "example.com");
        etest::expect(!url->port.has_value());
        etest::expect_eq(std::get<1>(url->path)[0], "login.php");
        etest::expect(!url->query.has_value());
        etest::expect(!url->fragment.has_value());

        etest::expect_eq(url->serialize(), "https://zero-one:testpass123@example.com/login.php");
    });

    etest::test("URL parsing: query", [] {
        url::UrlParser p;

        std::optional<url::Url> url =
                p.parse("https://www.youtube.com/watch?v=2g5xkLqIElUlist=PLHwvDXmNUa92NlFPooY1P5tfDo4T85ORzindex=3");

        etest::require(url.has_value());

        etest::expect_eq(url->scheme, "https");
        etest::expect_eq(std::get<0>(url->host->data), "www.youtube.com");
        etest::expect(!url->port.has_value());
        etest::expect_eq(std::get<1>(url->path)[0], "watch");
        etest::expect_eq(url->query, "v=2g5xkLqIElUlist=PLHwvDXmNUa92NlFPooY1P5tfDo4T85ORzindex=3");
        etest::expect(!url->fragment.has_value());

        etest::expect_eq(url->serialize(),
                "https://www.youtube.com/watch?v=2g5xkLqIElUlist=PLHwvDXmNUa92NlFPooY1P5tfDo4T85ORzindex=3");
    });

    etest::test("URL parsing: Welsh", [] {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse(
                "https://llanfairpwllgwyngyllgogerychwyrndrobwllllantysiliogogogoch.co.uk/images/platformticket.gif");

        etest::require(url.has_value());

        etest::expect_eq(url->scheme, "https");
        etest::expect_eq(
                std::get<0>(url->host->data), "llanfairpwllgwyngyllgogerychwyrndrobwllllantysiliogogogoch.co.uk");
        etest::expect(!url->port.has_value());
        etest::expect_eq(std::get<1>(url->path)[0], "images");
        etest::expect_eq(std::get<1>(url->path)[1], "platformticket.gif");
        etest::expect(!url->query.has_value());
        etest::expect(!url->fragment.has_value());

        etest::expect_eq(url->serialize(),
                "https://llanfairpwllgwyngyllgogerychwyrndrobwllllantysiliogogogoch.co.uk/images/platformticket.gif");
    });

    // This domain exceeds the maximum length of both a domain component/label and a FQDN
    etest::test("URL parsing: extreme Welsh", [] {
        url::UrlParser p;

        std::optional<url::Url> url =
                p.parse("https://"
                        "llanfairpwllgwyngyllgogerychwyrndrobwllllantysiliogogogochobwllllantysiliogogogochanfairpwllgw"
                        "yngyllgogerychgogerychwyrndrobwllllantysiliogogogochobwllllantysiliogogogochllanfairpwllgwyngy"
                        "llgogerychwyrndrobwllllantysiliogogogochobwllllantysiliogogogochanfairpwllgwyngyllgogerychgoge"
                        "rychwyrndrobwllllantysiliogogogochobwllllantysiliogogogoch.co.uk");

        etest::require(url.has_value());

        etest::expect_eq(url->scheme, "https");
        etest::expect_eq(std::get<0>(url->host->data),
                "llanfairpwllgwyngyllgogerychwyrndrobwllllantysiliogogogochobwllllantysiliogogogochanfairpwllgwyngyllgo"
                "gerychgogerychwyrndrobwllllantysiliogogogochobwllllantysiliogogogochllanfairpwllgwyngyllgogerychwyrndr"
                "obwllllantysiliogogogochobwllllantysiliogogogochanfairpwllgwyngyllgogerychgogerychwyrndrobwllllantysil"
                "iogogogochobwllllantysiliogogogoch.co.uk");
        etest::expect(!url->port.has_value());
        etest::expect_eq(std::get<1>(url->path)[0], "");
        etest::expect(!url->query.has_value());
        etest::expect(!url->fragment.has_value());

        etest::expect_eq(url->serialize(),
                "https://"
                "llanfairpwllgwyngyllgogerychwyrndrobwllllantysiliogogogochobwllllantysiliogogogochanfairpwllgw"
                "yngyllgogerychgogerychwyrndrobwllllantysiliogogogochobwllllantysiliogogogochllanfairpwllgwyngy"
                "llgogerychwyrndrobwllllantysiliogogogochobwllllantysiliogogogochanfairpwllgwyngyllgogerychgoge"
                "rychwyrndrobwllllantysiliogogogochobwllllantysiliogogogoch.co.uk/");
    });

    etest::test("URL parsing: path, query, and fragment", [] {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse(
                "https://github.com/robinlinden/hastur/actions/runs/4441133331/jobs/7795829478?pr=476#step:7:31");

        etest::require(url.has_value());

        etest::expect_eq(url->scheme, "https");
        etest::expect_eq(std::get<0>(url->host->data), "github.com");
        etest::expect(!url->port.has_value());
        etest::expect_eq(std::get<1>(url->path)[0], "robinlinden");
        etest::expect_eq(std::get<1>(url->path)[1], "hastur");
        etest::expect_eq(std::get<1>(url->path)[2], "actions");
        etest::expect_eq(std::get<1>(url->path)[3], "runs");
        etest::expect_eq(std::get<1>(url->path)[4], "4441133331");
        etest::expect_eq(std::get<1>(url->path)[5], "jobs");
        etest::expect_eq(std::get<1>(url->path)[6], "7795829478");
        etest::expect_eq(url->query, "pr=476");
        etest::expect_eq(url->fragment, "step:7:31");

        etest::expect_eq(url->serialize(),
                "https://github.com/robinlinden/hastur/actions/runs/4441133331/jobs/7795829478?pr=476#step:7:31");
    });

    etest::test("URL parsing: ipv4 and port", [] {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("https://127.0.0.1:631");

        etest::require(url.has_value());

        etest::expect_eq(url->scheme, "https");
        etest::expect_eq(std::get<1>(url->host->data), 2130706433ul);
        etest::expect_eq(url->port, 631);
        etest::expect_eq(std::get<1>(url->path)[0], "");
        etest::expect(!url->query.has_value());
        etest::expect(!url->fragment.has_value());

        etest::expect_eq(url->serialize(), "https://127.0.0.1:631/");
    });

    etest::test("URL parsing: ipv6 and port", [] {
        url::UrlParser p;

        const std::array<std::uint16_t, 8> addr{0x2001, 0xdb8, 0x85a3, 0, 0, 0x8a2e, 0x370, 0x7334};

        std::optional<url::Url> url = p.parse("https://[2001:db8:85a3::8a2e:370:7334]:631");

        etest::require(url.has_value());

        etest::expect_eq(url->scheme, "https");
        etest::expect_eq(std::get<2>(url->host->data), addr);
        etest::expect_eq(url->port, 631);
        etest::expect_eq(std::get<1>(url->path)[0], "");
        etest::expect(!url->query.has_value());
        etest::expect(!url->fragment.has_value());

        etest::expect_eq(url->serialize(), "https://[2001:db8:85a3::8a2e:370:7334]:631/");
    });

    etest::test("URL parsing: ipv6 v4-mapped with port", [] {
        url::UrlParser p;

        const std::array<std::uint16_t, 8> addr{0, 0, 0, 0, 0, 0xffff, 0x4ccb, 0x8c22};

        std::optional<url::Url> url = p.parse("https://[0000:0000:0000:0000:0000:ffff:4ccb:8c22]:631");

        etest::require(url.has_value());

        etest::expect_eq(url->scheme, "https");
        etest::expect_eq(std::get<2>(url->host->data), addr);
        etest::expect_eq(url->port, 631);
        etest::expect_eq(std::get<1>(url->path)[0], "");
        etest::expect(!url->query.has_value());
        etest::expect(!url->fragment.has_value());

        etest::expect_eq(url->serialize(), "https://[::ffff:4ccb:8c22]:631/");
    });

    etest::test("URL parsing: ipv6 v4-mapped compressed with dot-decimal", [] {
        url::UrlParser p;

        const std::array<std::uint16_t, 8> addr{0, 0, 0, 0, 0, 0xffff, 0x4ccb, 0x8c22};

        std::optional<url::Url> url = p.parse("https://[::ffff:76.203.140.34]:631");

        etest::require(url.has_value());

        etest::expect_eq(url->scheme, "https");
        etest::expect_eq(std::get<2>(url->host->data), addr);
        etest::expect_eq(url->port, 631);
        etest::expect_eq(std::get<1>(url->path)[0], "");
        etest::expect(!url->query.has_value());
        etest::expect(!url->fragment.has_value());

        etest::expect_eq(url->serialize(), "https://[::ffff:4ccb:8c22]:631/");
    });

    etest::test("URL parsing: empty input", [] {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("");

        etest::expect(!url.has_value());
    });

    etest::test("URL parsing: empty input with base URL", [&base] {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("", base);

        etest::require(url.has_value());

        etest::expect_eq(url->scheme, "https");
        etest::expect_eq(std::get<0>(url->host->data), "example.com");
        etest::expect_eq(url->port, 8080);
        etest::expect_eq(std::get<1>(url->path)[0], "test");
        etest::expect_eq(std::get<1>(url->path)[1], "index.php");
        etest::expect(!url->query.has_value());
        etest::expect(!url->fragment.has_value());

        etest::expect_eq(url->serialize(), "https://example.com:8080/test/index.php");
    });

    etest::test("URL parsing: query input with base URL", [&base] {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("?view=table", base);

        etest::require(url.has_value());

        etest::expect_eq(url->scheme, "https");
        etest::expect_eq(std::get<0>(url->host->data), "example.com");
        etest::expect_eq(url->port, 8080);
        etest::expect_eq(std::get<1>(url->path)[0], "test");
        etest::expect_eq(std::get<1>(url->path)[1], "index.php");
        etest::expect_eq(url->query, "view=table");
        etest::expect(!url->fragment.has_value());

        etest::expect_eq(url->serialize(), "https://example.com:8080/test/index.php?view=table");
    });

    etest::test("URL parsing: file URL", [] {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("file:///home/zero-one/repos/hastur/README.md");

        etest::require(url.has_value());

        etest::expect_eq(url->scheme, "file");
        etest::expect_eq(std::get<0>(url->host->data), "");
        etest::expect(!url->port.has_value());
        etest::expect_eq(std::get<1>(url->path)[0], "home");
        etest::expect_eq(std::get<1>(url->path)[1], "zero-one");
        etest::expect_eq(std::get<1>(url->path)[2], "repos");
        etest::expect_eq(std::get<1>(url->path)[3], "hastur");
        etest::expect_eq(std::get<1>(url->path)[4], "README.md");
        etest::expect(!url->query.has_value());
        etest::expect(!url->fragment.has_value());

        etest::expect_eq(url->serialize(), "file:///home/zero-one/repos/hastur/README.md");
    });

    etest::test("URL parsing: file URL with double-dot", [] {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("file:///home/zero-one/repos/../hastur/README.md");

        etest::require(url.has_value());

        etest::expect_eq(url->scheme, "file");
        etest::expect_eq(std::get<0>(url->host->data), "");
        etest::expect(!url->port.has_value());
        etest::expect_eq(std::get<1>(url->path)[0], "home");
        etest::expect_eq(std::get<1>(url->path)[1], "zero-one");
        etest::expect_eq(std::get<1>(url->path)[2], "hastur");
        etest::expect_eq(std::get<1>(url->path)[3], "README.md");
        etest::expect(!url->query.has_value());
        etest::expect(!url->fragment.has_value());

        etest::expect_eq(url->serialize(), "file:///home/zero-one/hastur/README.md");
    });

    etest::test("URL parsing: file URL with double-dot 2", [] {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("file:///home/zero-one/repos/../hastur/../README.md");

        etest::require(url.has_value());

        etest::expect_eq(url->scheme, "file");
        etest::expect_eq(std::get<0>(url->host->data), "");
        etest::expect(!url->port.has_value());
        etest::expect_eq(std::get<1>(url->path)[0], "home");
        etest::expect_eq(std::get<1>(url->path)[1], "zero-one");
        etest::expect_eq(std::get<1>(url->path)[2], "README.md");
        etest::expect(!url->query.has_value());
        etest::expect(!url->fragment.has_value());

        etest::expect_eq(url->serialize(), "file:///home/zero-one/README.md");
    });

    etest::test("URL parsing: file URL with double-dot 3", [] {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("file:///../home/zero-one/repos/");

        etest::require(url.has_value());

        etest::expect_eq(url->scheme, "file");
        etest::expect_eq(std::get<0>(url->host->data), "");
        etest::expect(!url->port.has_value());
        etest::expect_eq(std::get<1>(url->path)[0], "home");
        etest::expect_eq(std::get<1>(url->path)[1], "zero-one");
        etest::expect_eq(std::get<1>(url->path)[2], "repos");
        etest::expect(!url->query.has_value());
        etest::expect(!url->fragment.has_value());

        etest::expect_eq(url->serialize(), "file:///home/zero-one/repos/");
    });

    etest::test("URL parsing: file URL with single-dot", [] {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("file:///home/zero-one/repos/./hastur/README.md");

        etest::require(url.has_value());

        etest::expect_eq(url->scheme, "file");
        etest::expect_eq(std::get<0>(url->host->data), "");
        etest::expect(!url->port.has_value());
        etest::expect_eq(std::get<1>(url->path)[0], "home");
        etest::expect_eq(std::get<1>(url->path)[1], "zero-one");
        etest::expect_eq(std::get<1>(url->path)[2], "repos");
        etest::expect_eq(std::get<1>(url->path)[3], "hastur");
        etest::expect_eq(std::get<1>(url->path)[4], "README.md");
        etest::expect(!url->query.has_value());
        etest::expect(!url->fragment.has_value());

        etest::expect_eq(url->serialize(), "file:///home/zero-one/repos/hastur/README.md");
    });

    etest::test("URL parsing: file URL with windows path", [] {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse(R"(file://C:\Users\zero-one\repos\hastur\README.md)");

        etest::require(url.has_value());

        etest::expect_eq(url->scheme, "file");
        etest::expect_eq(std::get<0>(url->host->data), "");
        etest::expect(!url->port.has_value());
        etest::expect_eq(std::get<1>(url->path)[0], "C:");
        etest::expect_eq(std::get<1>(url->path)[1], "Users");
        etest::expect_eq(std::get<1>(url->path)[2], "zero-one");
        etest::expect_eq(std::get<1>(url->path)[3], "repos");
        etest::expect_eq(std::get<1>(url->path)[4], "hastur");
        etest::expect_eq(std::get<1>(url->path)[5], "README.md");
        etest::expect(!url->query.has_value());
        etest::expect(!url->fragment.has_value());

        etest::expect_eq(url->serialize(), R"(file:///C:/Users/zero-one/repos/hastur/README.md)");
    });

    etest::test("URL origin", [] {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("https://example.com:8080/index.html");
        std::optional<url::Url> url2 = p.parse("https://example.com:9999/index.php");
        std::optional<url::Url> url3 = p.parse("http://example.com:8080/index.html");
        std::optional<url::Url> url4 = p.parse("https://example.com:8080/index.php?foo=bar");

        etest::require(url.has_value());
        etest::require(url2.has_value());
        etest::require(url3.has_value());
        etest::require(url4.has_value());

        url::Origin o = url->origin();
        url::Origin o2 = url2->origin();
        url::Origin o3 = url3->origin();
        url::Origin o4 = url4->origin();
        url::Origin o5{"https", {url::HostType::DnsDomain, "example.com"}, std::uint16_t{8080}, "example.com"};

        etest::require(o.port.has_value());
        etest::require(o2.port.has_value());
        etest::require(o3.port.has_value());
        etest::require(o4.port.has_value());

        etest::expect(!o.domain.has_value());
        etest::expect(!o2.domain.has_value());
        etest::expect(!o3.domain.has_value());
        etest::expect(!o4.domain.has_value());

        etest::expect_eq(o.scheme, "https");
        etest::expect_eq(o2.scheme, "https");
        etest::expect_eq(o3.scheme, "http");
        etest::expect_eq(o4.scheme, "https");

        etest::expect_eq(o.host.serialize(), "example.com");
        etest::expect_eq(o2.host.serialize(), "example.com");
        etest::expect_eq(o3.host.serialize(), "example.com");
        etest::expect_eq(o4.host.serialize(), "example.com");

        etest::expect_eq(*o.port, 8080);
        etest::expect_eq(*o2.port, 9999);
        etest::expect_eq(*o3.port, 8080);
        etest::expect_eq(*o4.port, 8080);

        etest::expect(!o.opaque);
        etest::expect(!o2.opaque);
        etest::expect(!o3.opaque);
        etest::expect(!o4.opaque);

        etest::expect_eq(o.serialize(), "https://example.com:8080");
        etest::expect_eq(o2.serialize(), "https://example.com:9999");
        etest::expect_eq(o3.serialize(), "http://example.com:8080");
        etest::expect_eq(o4.serialize(), "https://example.com:8080");

        etest::expect(o != o2);
        etest::expect(o != o3);
        etest::expect(o == o4);
        etest::expect(o == o5);

        etest::expect(!o.is_same_origin_domain(o2));
        etest::expect(!o.is_same_origin_domain(o3));
        etest::expect(o.is_same_origin_domain(o4));
        etest::expect(!o.is_same_origin_domain(o5));

        etest::expect(std::holds_alternative<url::Host>(o.effective_domain()));
        etest::expect(std::holds_alternative<url::Host>(o2.effective_domain()));
        etest::expect(std::holds_alternative<url::Host>(o3.effective_domain()));
        etest::expect(std::holds_alternative<url::Host>(o4.effective_domain()));
        etest::expect(std::holds_alternative<std::string>(o5.effective_domain()));

        etest::expect_eq(std::get<std::string>(o5.effective_domain()), "example.com");
    });

    etest::test("URL origin: opaque origin", [] {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("file:///usr/local/bin/foo");
        std::optional<url::Url> url2 = p.parse("file:///etc/passwd");
        std::optional<url::Url> url3 = p.parse("http://example.com");

        etest::require(url.has_value());
        etest::require(url2.has_value());
        etest::require(url3.has_value());

        url::Origin o = url->origin();
        url::Origin o2 = url2->origin();
        url::Origin o3 = url3->origin();

        etest::expect(o.opaque);
        etest::expect(o2.opaque);
        etest::expect(!o3.opaque);

        etest::expect_eq(o.serialize(), "null");
        etest::expect_eq(o2.serialize(), "null");

        etest::expect(std::holds_alternative<std::monostate>(o.effective_domain()));
        etest::expect(std::holds_alternative<std::monostate>(o2.effective_domain()));

        etest::expect(o == o2);
        etest::expect(o != o3);

        etest::expect(o.is_same_origin_domain(o2));
        etest::expect(!o.is_same_origin_domain(o3));
    });

    etest::test("URL origin: blob URL", [] {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("blob:https://whatwg.org/d0360e2f-caee-469f-9a2f-87d5b0456f6f");
        std::optional<url::Url> url2 = p.parse("blob:ws://whatwg.org/d0360e2f-caee-469f-9a2f-87d5b0456f6f");

        etest::require(url.has_value());
        etest::require(url2.has_value());

        url::Origin o = url->origin();
        url::Origin o2 = url2->origin();

        etest::expect(!o.opaque);
        etest::expect(o2.opaque);

        etest::expect(!o.port.has_value());
        etest::expect(!o.domain.has_value());

        etest::expect_eq(o.scheme, "https");
        etest::expect_eq(o.host.serialize(), "whatwg.org");

        etest::expect_eq(o.serialize(), "https://whatwg.org");
        etest::expect_eq(o2.serialize(), "null");
    });

    etest::test("URL parsing: parse_host w/ empty input", [] {
        url::UrlParser p;
        auto url = p.parse("a://");

        etest::require(url.has_value());
        etest::expect_eq(*url, url::Url{.scheme = "a", .host = url::Host{.type = url::HostType::Opaque}});
    });

    int ret = etest::run_all_tests();

    url::icu_cleanup();

    return ret;
}
