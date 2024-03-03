// SPDX-FileCopyrightText: 2023 David Zero <zero-one@zer0-one.net>
// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "url/url.h"

#include "etest/etest.h"

#include <simdjson.h> // IWYU pragma: keep

#include <array>
#include <cstdint>
#include <iostream>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace {

struct ParseResult {
    std::optional<url::Url> url;
    std::vector<url::ValidationError> errors;
};

ParseResult parse_url(std::string input, std::optional<url::Url> base = std::nullopt) {
    std::vector<url::ValidationError> errors;
    url::UrlParser p;
    p.set_on_error([&errors](url::ValidationError e) { errors.push_back(e); });
    std::optional<url::Url> url = p.parse(std::move(input), std::move(base));
    return {std::move(url), std::move(errors)};
}

} // namespace

int main() {
    url::Url const base{"https",
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
        std::cout << "\nGenerated Blob URL: " << blob << '\n';

        etest::expect(std::regex_match(blob, std::regex("blob:https://example.com:8080/" + regex_uuid)));

        h = url::Host{url::HostType::Ip4Addr, std::uint32_t{134744072}};
        o = {"https", h, std::uint16_t{8080}, std::nullopt, false};

        blob = url::blob_url_create(o);
        std::cout << "Generated Blob URL: " << blob << '\n';

        etest::expect(std::regex_match(blob, std::regex("blob:https://8.8.8.8:8080/" + regex_uuid)));

        std::array<std::uint16_t, 8> v6 = {0x2001, 0xdb8, 0x85a3, 0, 0, 0x8a2e, 0x370, 0x7334};
        h = url::Host{url::HostType::Ip6Addr, v6};
        o = {"https", h, std::uint16_t{8080}, std::nullopt, false};

        blob = url::blob_url_create(o);
        std::cout << "Generated Blob URL: " << blob << '\n';

        etest::expect(std::regex_match(
                blob, std::regex("blob:https://\\[2001:db8:85a3::8a2e:370:7334\\]:8080/" + regex_uuid)));
    });

    etest::test("Validation error: description", [] {
        etest::expect(!description(url::ValidationError::DomainInvalidCodePoint).empty()); //
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

    etest::test("URL parsing: file url with base", [] {
        url::UrlParser p;

        std::optional<url::Url> file_base = p.parse("file:///usr/bin/vim");

        etest::require(file_base.has_value());

        std::optional<url::Url> url = p.parse("file:usr/bin/emacs", file_base);

        etest::require(url.has_value());

        etest::expect_eq(url->scheme, "file");
        etest::expect_eq(url->serialize(), "file:///usr/bin/usr/bin/emacs");
        etest::expect_eq(url->host->serialize(), "");
        etest::expect_eq(url->serialize_path(), "/usr/bin/usr/bin/emacs");
    });

    etest::test("URL parsing: file url backslash with base", [] {
        url::UrlParser p;

        std::optional<url::Url> file_base = p.parse("file:///usr/bin/vim");

        etest::require(file_base.has_value());

        std::optional<url::Url> url = p.parse("file:\\usr/bin/emacs", file_base);

        etest::require(url.has_value());

        etest::expect_eq(url->scheme, "file");
        etest::expect_eq(url->serialize(), "file:///usr/bin/emacs");
        etest::expect_eq(url->host->serialize(), "");
        etest::expect_eq(url->serialize_path(), "/usr/bin/emacs");
    });

    etest::test("URL parsing: non-relative url w/o scheme", [] {
        auto [url, errors] = parse_url("//example.com");
        etest::expect_eq(url, std::nullopt);
        etest::expect_eq(errors, std::vector{url::ValidationError::MissingSchemeNonRelativeUrl});
    });

    etest::test("URL normalization: uppercasing percent-encoded triplets", [] {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("http://example.com/foo%2a");

        etest::require(url.has_value());

        etest::expect_eq(url->serialize(false, true), "http://example.com/foo%2A");
    });

    etest::test("URL normalization: lowercasing scheme and host", [] {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("HTTP://User@Example.COM/Foo");

        etest::require(url.has_value());

        etest::expect_eq(url->serialize(), "http://User@example.com/Foo");
    });

    etest::test("URL normalization: decoding percent-encoded triplets of unreserved characters", [] {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("http://example.com/%7Efoo");

        etest::require(url.has_value());

        etest::expect_eq(url->serialize(false, true), "http://example.com/~foo");
    });

    etest::test("URL normalization: removing dot-segments", [] {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("http://example.com/foo/./bar/baz/../qux");

        etest::require(url.has_value());

        etest::expect_eq(url->serialize(), "http://example.com/foo/bar/qux");
    });

    etest::test("URL normalization: converting empty path to '/'", [] {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("http://example.com");

        etest::require(url.has_value());

        etest::expect_eq(url->serialize(), "http://example.com/");
    });

    etest::test("URL normalization: removing default port", [] {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("http://example.com:80/");

        etest::require(url.has_value());

        etest::expect_eq(url->serialize(), "http://example.com/");
    });

    // NOLINTBEGIN(misc-include-cleaner): What you're meant to include from
    // simdjson depends on things like the architecture you're compiling for.
    // This is handled automagically with detection macros inside simdjson.
    etest::test("Web Platform Tests", [] {
        url::UrlParser p;

        simdjson::ondemand::parser parser;

        auto json = simdjson::padded_string::load("../wpt/url/resources/urltestdata.json");

        simdjson::ondemand::document doc = parser.iterate(json);

        simdjson::ondemand::array arr = doc.get_array();

        for (auto obj : arr) {
            // Skip strings, those are just comments
            if (obj.type() == simdjson::ondemand::json_type::string) {
                continue;
            }

            bool should_fail = false;

            // Check if test expects failure
            if (obj.find_field("failure").error() != simdjson::error_code::NO_SUCH_FIELD) {
                should_fail = true;
            }

            // Get input URL
            std::string_view input = obj["input"].get_string(true);

            // Parse base URL if it exists
            std::optional<url::Url> base_test;

            if (!obj["base"].is_null()) {
                std::string_view base_str = obj["base"].get_string(true);

                base_test = p.parse(std::string{base_str});

                if (!should_fail) {
                    etest::expect(base_test.has_value(), "Parsing base URL:(" + std::string{base_str} + ") failed");

                    continue;
                }
            }

            // Parse input URL
            std::optional<url::Url> url = p.parse(std::string{input}, base_test);

            if (!should_fail) {
                etest::expect(url.has_value(), "Parsing input URL:(" + std::string{input} + ") failed");

                if (!url.has_value()) {
                    continue;
                }
            } else {
                etest::require(!url.has_value(),
                        "Parsing input URL:(" + std::string{input} + ") succeeded when it was supposed to fail");

                // If this test was an expected failure, test ends here
                continue;
            }

            // Check URL fields against test

            std::string_view href = obj["href"];
            etest::expect_eq(url->serialize(), href);

            if (obj.find_field("failure").error() != simdjson::error_code::NO_SUCH_FIELD) {
                std::string_view origin = obj["origin"];

                etest::expect_eq(url->origin().serialize(), origin);
            }

            std::string_view protocol = obj["protocol"];
            etest::expect_eq(url->scheme + ":", protocol);

            std::string_view username = obj["username"];
            etest::expect_eq(url->user, username);

            std::string_view password = obj["password"];
            etest::expect_eq(url->passwd, password);

            std::string_view hostname = obj["hostname"];
            etest::expect_eq(url->host.has_value() ? url->host->serialize() : "", hostname);

            std::string_view host = obj["host"];
            std::string host_serialized = url->host.has_value() ? url->host->serialize() : "";
            std::string host_port = url->port.has_value() ? std::string{":"} + std::to_string(*url->port) : "";
            etest::expect_eq(host_serialized + host_port, host);

            std::string_view port = obj["port"];
            etest::expect_eq(url->port.has_value() ? std::to_string(*url->port) : "", port);

            std::string_view pathname = obj["pathname"];
            etest::expect_eq(url->serialize_path(), pathname);

            std::string_view search = obj["search"];
            etest::expect_eq(
                    url->query.has_value() && !url->query->empty() ? std::string{"?"} + *url->query : "", search);

            std::string_view hash = obj["hash"];
            etest::expect_eq(
                    url->fragment.has_value() && !url->fragment->empty() ? std::string{"#"} + *url->fragment : "",
                    hash);
        }
    });
    // NOLINTEND(misc-include-cleaner)

    int ret = etest::run_all_tests();

    url::icu_cleanup();

    return ret;
}
