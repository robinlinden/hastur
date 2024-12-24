// SPDX-FileCopyrightText: 2023 David Zero <zero-one@zer0-one.net>
// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "url/url.h"

#include "etest/etest2.h"

#include <simdjson.h> // IWYU pragma: keep

#include <array>
#include <cstdint>
#include <iostream>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <tuple>
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
    etest::Suite s{};

    url::Url const base{"https",
            "",
            "",
            url::Host{url::HostType::DnsDomain, "example.com"},
            std::uint16_t{8080},
            std::vector<std::string>{"test", "index.php"}};

    s.add_test("blob URL generation", [](etest::IActions &a) {
        std::string regex_uuid = "[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}";

        url::Host h = {url::HostType::DnsDomain, "example.com"};
        url::Origin o = {"https", h, std::uint16_t{8080}, std::nullopt, false};

        std::string blob = url::blob_url_create(o);
        std::cout << "\nGenerated Blob URL: " << blob << '\n';

        a.expect(std::regex_match(blob, std::regex("blob:https://example.com:8080/" + regex_uuid)));

        h = url::Host{url::HostType::Ip4Addr, std::uint32_t{134744072}};
        o = {"https", h, std::uint16_t{8080}, std::nullopt, false};

        blob = url::blob_url_create(o);
        std::cout << "Generated Blob URL: " << blob << '\n';

        a.expect(std::regex_match(blob, std::regex("blob:https://8.8.8.8:8080/" + regex_uuid)));

        std::array<std::uint16_t, 8> v6 = {0x2001, 0xdb8, 0x85a3, 0, 0, 0x8a2e, 0x370, 0x7334};
        h = url::Host{url::HostType::Ip6Addr, v6};
        o = {"https", h, std::uint16_t{8080}, std::nullopt, false};

        blob = url::blob_url_create(o);
        std::cout << "Generated Blob URL: " << blob << '\n';

        a.expect(std::regex_match(
                blob, std::regex("blob:https://\\[2001:db8:85a3::8a2e:370:7334\\]:8080/" + regex_uuid)));
    });

    s.add_test("Validation error: description", [](etest::IActions &a) {
        a.expect(!description(url::ValidationError::DomainInvalidCodePoint).empty()); //
    });

    s.add_test("URL parsing: port and path", [](etest::IActions &a) {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("https://example.com:8080/index.html");

        a.require(url.has_value());

        a.expect_eq(url->scheme, "https");
        a.expect_eq(std::get<0>(url->host->data), "example.com");
        a.expect_eq(url->port.value(), 8080);
        a.expect_eq(std::get<1>(url->path)[0], "index.html");
        a.expect(!url->query.has_value());
        a.expect(!url->fragment.has_value());

        a.expect_eq(url->serialize(), "https://example.com:8080/index.html");
    });

    s.add_test("URL parsing: 1 unicode char", [](etest::IActions &a) {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("http://bücher.de");

        a.require(url.has_value());

        a.expect_eq(url->scheme, "http");
        a.expect_eq(std::get<0>(url->host->data), "xn--bcher-kva.de");
        a.expect(!url->port.has_value());
        a.expect_eq(std::get<1>(url->path)[0], "");
        a.expect(!url->query.has_value());
        a.expect(!url->fragment.has_value());

        a.expect_eq(url->serialize(), "http://xn--bcher-kva.de/");
    });

    s.add_test("URL parsing: 1 unicode char with path", [](etest::IActions &a) {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("https://√.com/i/itunes.gif");

        a.require(url.has_value());

        a.expect_eq(url->scheme, "https");
        a.expect_eq(std::get<0>(url->host->data), "xn--19g.com");
        a.expect(!url->port.has_value());
        a.expect_eq(std::get<1>(url->path)[0], "i");
        a.expect_eq(std::get<1>(url->path)[1], "itunes.gif");
        a.expect(!url->query.has_value());
        a.expect(!url->fragment.has_value());

        a.expect_eq(url->serialize(), "https://xn--19g.com/i/itunes.gif");
    });

    s.add_test("URL parsing: unicode path", [](etest::IActions &a) {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("https://ar.wikipedia.org/wiki/نجيب_محفوظ");

        a.require(url.has_value());

        a.expect_eq(url->scheme, "https");
        a.expect_eq(std::get<0>(url->host->data), "ar.wikipedia.org");
        a.expect(!url->port.has_value());
        a.expect_eq(std::get<1>(url->path)[0], "wiki");
        a.expect_eq(std::get<1>(url->path)[1], "%D9%86%D8%AC%D9%8A%D8%A8_%D9%85%D8%AD%D9%81%D9%88%D8%B8");
        a.expect(!url->query.has_value());
        a.expect(!url->fragment.has_value());

        a.expect_eq(url->serialize(),
                "https://ar.wikipedia.org/wiki/%D9%86%D8%AC%D9%8A%D8%A8_%D9%85%D8%AD%D9%81%D9%88%D8%B8");
    });

    s.add_test("URL parsing: tel URI", [](etest::IActions &a) {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("tel:+1-555-555-5555");

        a.require(url.has_value());

        a.expect_eq(url->scheme, "tel");
        a.expect(!url->host.has_value());
        a.expect(!url->port.has_value());
        a.expect_eq(std::get<0>(url->path), "+1-555-555-5555");
        a.expect(!url->query.has_value());
        a.expect(!url->fragment.has_value());

        a.expect_eq(url->serialize(), "tel:+1-555-555-5555");
    });

    s.add_test("URL parsing: username and passwd in authority", [](etest::IActions &a) {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("https://zero-one:testpass123@example.com/login.php");

        a.require(url.has_value());

        a.expect_eq(url->scheme, "https");
        a.expect_eq(url->user, "zero-one");
        a.expect_eq(url->passwd, "testpass123");
        a.expect_eq(std::get<0>(url->host->data), "example.com");
        a.expect(!url->port.has_value());
        a.expect_eq(std::get<1>(url->path)[0], "login.php");
        a.expect(!url->query.has_value());
        a.expect(!url->fragment.has_value());

        a.expect_eq(url->serialize(), "https://zero-one:testpass123@example.com/login.php");
    });

    s.add_test("URL parsing: query", [](etest::IActions &a) {
        url::UrlParser p;

        std::optional<url::Url> url =
                p.parse("https://www.youtube.com/watch?v=2g5xkLqIElUlist=PLHwvDXmNUa92NlFPooY1P5tfDo4T85ORzindex=3");

        a.require(url.has_value());

        a.expect_eq(url->scheme, "https");
        a.expect_eq(std::get<0>(url->host->data), "www.youtube.com");
        a.expect(!url->port.has_value());
        a.expect_eq(std::get<1>(url->path)[0], "watch");
        a.expect_eq(url->query, "v=2g5xkLqIElUlist=PLHwvDXmNUa92NlFPooY1P5tfDo4T85ORzindex=3");
        a.expect(!url->fragment.has_value());

        a.expect_eq(url->serialize(),
                "https://www.youtube.com/watch?v=2g5xkLqIElUlist=PLHwvDXmNUa92NlFPooY1P5tfDo4T85ORzindex=3");
    });

    s.add_test("URL parsing: Welsh", [](etest::IActions &a) {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse(
                "https://llanfairpwllgwyngyllgogerychwyrndrobwllllantysiliogogogoch.co.uk/images/platformticket.gif");

        a.require(url.has_value());

        a.expect_eq(url->scheme, "https");
        a.expect_eq(std::get<0>(url->host->data), "llanfairpwllgwyngyllgogerychwyrndrobwllllantysiliogogogoch.co.uk");
        a.expect(!url->port.has_value());
        a.expect_eq(std::get<1>(url->path)[0], "images");
        a.expect_eq(std::get<1>(url->path)[1], "platformticket.gif");
        a.expect(!url->query.has_value());
        a.expect(!url->fragment.has_value());

        a.expect_eq(url->serialize(),
                "https://llanfairpwllgwyngyllgogerychwyrndrobwllllantysiliogogogoch.co.uk/images/platformticket.gif");
    });

    // This domain exceeds the maximum length of both a domain component/label and a FQDN
    s.add_test("URL parsing: extreme Welsh", [](etest::IActions &a) {
        url::UrlParser p;

        std::optional<url::Url> url =
                p.parse("https://"
                        "llanfairpwllgwyngyllgogerychwyrndrobwllllantysiliogogogochobwllllantysiliogogogochanfairpwllgw"
                        "yngyllgogerychgogerychwyrndrobwllllantysiliogogogochobwllllantysiliogogogochllanfairpwllgwyngy"
                        "llgogerychwyrndrobwllllantysiliogogogochobwllllantysiliogogogochanfairpwllgwyngyllgogerychgoge"
                        "rychwyrndrobwllllantysiliogogogochobwllllantysiliogogogoch.co.uk");

        a.require(url.has_value());

        a.expect_eq(url->scheme, "https");
        a.expect_eq(std::get<0>(url->host->data),
                "llanfairpwllgwyngyllgogerychwyrndrobwllllantysiliogogogochobwllllantysiliogogogochanfairpwllgwyngyllgo"
                "gerychgogerychwyrndrobwllllantysiliogogogochobwllllantysiliogogogochllanfairpwllgwyngyllgogerychwyrndr"
                "obwllllantysiliogogogochobwllllantysiliogogogochanfairpwllgwyngyllgogerychgogerychwyrndrobwllllantysil"
                "iogogogochobwllllantysiliogogogoch.co.uk");
        a.expect(!url->port.has_value());
        a.expect_eq(std::get<1>(url->path)[0], "");
        a.expect(!url->query.has_value());
        a.expect(!url->fragment.has_value());

        a.expect_eq(url->serialize(),
                "https://"
                "llanfairpwllgwyngyllgogerychwyrndrobwllllantysiliogogogochobwllllantysiliogogogochanfairpwllgw"
                "yngyllgogerychgogerychwyrndrobwllllantysiliogogogochobwllllantysiliogogogochllanfairpwllgwyngy"
                "llgogerychwyrndrobwllllantysiliogogogochobwllllantysiliogogogochanfairpwllgwyngyllgogerychgoge"
                "rychwyrndrobwllllantysiliogogogochobwllllantysiliogogogoch.co.uk/");
    });

    s.add_test("URL parsing: path, query, and fragment", [](etest::IActions &a) {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse(
                "https://github.com/robinlinden/hastur/actions/runs/4441133331/jobs/7795829478?pr=476#step:7:31");

        a.require(url.has_value());

        a.expect_eq(url->scheme, "https");
        a.expect_eq(std::get<0>(url->host->data), "github.com");
        a.expect(!url->port.has_value());
        a.expect_eq(std::get<1>(url->path)[0], "robinlinden");
        a.expect_eq(std::get<1>(url->path)[1], "hastur");
        a.expect_eq(std::get<1>(url->path)[2], "actions");
        a.expect_eq(std::get<1>(url->path)[3], "runs");
        a.expect_eq(std::get<1>(url->path)[4], "4441133331");
        a.expect_eq(std::get<1>(url->path)[5], "jobs");
        a.expect_eq(std::get<1>(url->path)[6], "7795829478");
        a.expect_eq(url->query, "pr=476");
        a.expect_eq(url->fragment, "step:7:31");

        a.expect_eq(url->serialize(),
                "https://github.com/robinlinden/hastur/actions/runs/4441133331/jobs/7795829478?pr=476#step:7:31");
    });

    s.add_test("URL parsing: ipv4 and port", [](etest::IActions &a) {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("https://127.0.0.1:631");

        a.require(url.has_value());

        a.expect_eq(url->scheme, "https");
        a.expect_eq(std::get<1>(url->host->data), 2130706433ul);
        a.expect_eq(url->port, 631);
        a.expect_eq(std::get<1>(url->path)[0], "");
        a.expect(!url->query.has_value());
        a.expect(!url->fragment.has_value());

        a.expect_eq(url->serialize(), "https://127.0.0.1:631/");
    });

    s.add_test("URL parsing: ipv6 and port", [](etest::IActions &a) {
        url::UrlParser p;

        std::array<std::uint16_t, 8> const addr{0x2001, 0xdb8, 0x85a3, 0, 0, 0x8a2e, 0x370, 0x7334};

        std::optional<url::Url> url = p.parse("https://[2001:db8:85a3::8a2e:370:7334]:631");

        a.require(url.has_value());

        a.expect_eq(url->scheme, "https");
        a.expect_eq(std::get<2>(url->host->data), addr);
        a.expect_eq(url->port, 631);
        a.expect_eq(std::get<1>(url->path)[0], "");
        a.expect(!url->query.has_value());
        a.expect(!url->fragment.has_value());

        a.expect_eq(url->serialize(), "https://[2001:db8:85a3::8a2e:370:7334]:631/");
    });

    s.add_test("URL parsing: ipv6 v4-mapped with port", [](etest::IActions &a) {
        url::UrlParser p;

        std::array<std::uint16_t, 8> const addr{0, 0, 0, 0, 0, 0xffff, 0x4ccb, 0x8c22};

        std::optional<url::Url> url = p.parse("https://[0000:0000:0000:0000:0000:ffff:4ccb:8c22]:631");

        a.require(url.has_value());

        a.expect_eq(url->scheme, "https");
        a.expect_eq(std::get<2>(url->host->data), addr);
        a.expect_eq(url->port, 631);
        a.expect_eq(std::get<1>(url->path)[0], "");
        a.expect(!url->query.has_value());
        a.expect(!url->fragment.has_value());

        a.expect_eq(url->serialize(), "https://[::ffff:4ccb:8c22]:631/");
    });

    s.add_test("URL parsing: ipv6 v4-mapped compressed with dot-decimal", [](etest::IActions &a) {
        url::UrlParser p;

        std::array<std::uint16_t, 8> const addr{0, 0, 0, 0, 0, 0xffff, 0x4ccb, 0x8c22};

        std::optional<url::Url> url = p.parse("https://[::ffff:76.203.140.34]:631");

        a.require(url.has_value());

        a.expect_eq(url->scheme, "https");
        a.expect_eq(std::get<2>(url->host->data), addr);
        a.expect_eq(url->port, 631);
        a.expect_eq(std::get<1>(url->path)[0], "");
        a.expect(!url->query.has_value());
        a.expect(!url->fragment.has_value());

        a.expect_eq(url->serialize(), "https://[::ffff:4ccb:8c22]:631/");
    });

    s.add_test("URL parsing: empty input", [](etest::IActions &a) {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("");

        a.expect(!url.has_value());
    });

    s.add_test("URL parsing: empty input with base URL", [&base](etest::IActions &a) {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("", base);

        a.require(url.has_value());

        a.expect_eq(url->scheme, "https");
        a.expect_eq(std::get<0>(url->host->data), "example.com");
        a.expect_eq(url->port, 8080);
        a.expect_eq(std::get<1>(url->path)[0], "test");
        a.expect_eq(std::get<1>(url->path)[1], "index.php");
        a.expect(!url->query.has_value());
        a.expect(!url->fragment.has_value());

        a.expect_eq(url->serialize(), "https://example.com:8080/test/index.php");
    });

    s.add_test("URL parsing: query input with base URL", [&base](etest::IActions &a) {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("?view=table", base);

        a.require(url.has_value());

        a.expect_eq(url->scheme, "https");
        a.expect_eq(std::get<0>(url->host->data), "example.com");
        a.expect_eq(url->port, 8080);
        a.expect_eq(std::get<1>(url->path)[0], "test");
        a.expect_eq(std::get<1>(url->path)[1], "index.php");
        a.expect_eq(url->query, "view=table");
        a.expect(!url->fragment.has_value());

        a.expect_eq(url->serialize(), "https://example.com:8080/test/index.php?view=table");
    });

    s.add_test("URL parsing: file URL", [](etest::IActions &a) {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("file:///home/zero-one/repos/hastur/README.md");

        a.require(url.has_value());

        a.expect_eq(url->scheme, "file");
        a.expect_eq(std::get<0>(url->host->data), "");
        a.expect(!url->port.has_value());
        a.expect_eq(std::get<1>(url->path)[0], "home");
        a.expect_eq(std::get<1>(url->path)[1], "zero-one");
        a.expect_eq(std::get<1>(url->path)[2], "repos");
        a.expect_eq(std::get<1>(url->path)[3], "hastur");
        a.expect_eq(std::get<1>(url->path)[4], "README.md");
        a.expect(!url->query.has_value());
        a.expect(!url->fragment.has_value());

        a.expect_eq(url->serialize(), "file:///home/zero-one/repos/hastur/README.md");
    });

    s.add_test("URL parsing: file URL with double-dot", [](etest::IActions &a) {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("file:///home/zero-one/repos/../hastur/README.md");

        a.require(url.has_value());

        a.expect_eq(url->scheme, "file");
        a.expect_eq(std::get<0>(url->host->data), "");
        a.expect(!url->port.has_value());
        a.expect_eq(std::get<1>(url->path)[0], "home");
        a.expect_eq(std::get<1>(url->path)[1], "zero-one");
        a.expect_eq(std::get<1>(url->path)[2], "hastur");
        a.expect_eq(std::get<1>(url->path)[3], "README.md");
        a.expect(!url->query.has_value());
        a.expect(!url->fragment.has_value());

        a.expect_eq(url->serialize(), "file:///home/zero-one/hastur/README.md");
    });

    s.add_test("URL parsing: file URL with double-dot 2", [](etest::IActions &a) {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("file:///home/zero-one/repos/../hastur/../README.md");

        a.require(url.has_value());

        a.expect_eq(url->scheme, "file");
        a.expect_eq(std::get<0>(url->host->data), "");
        a.expect(!url->port.has_value());
        a.expect_eq(std::get<1>(url->path)[0], "home");
        a.expect_eq(std::get<1>(url->path)[1], "zero-one");
        a.expect_eq(std::get<1>(url->path)[2], "README.md");
        a.expect(!url->query.has_value());
        a.expect(!url->fragment.has_value());

        a.expect_eq(url->serialize(), "file:///home/zero-one/README.md");
    });

    s.add_test("URL parsing: file URL with double-dot 3", [](etest::IActions &a) {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("file:///../home/zero-one/repos/");

        a.require(url.has_value());

        a.expect_eq(url->scheme, "file");
        a.expect_eq(std::get<0>(url->host->data), "");
        a.expect(!url->port.has_value());
        a.expect_eq(std::get<1>(url->path)[0], "home");
        a.expect_eq(std::get<1>(url->path)[1], "zero-one");
        a.expect_eq(std::get<1>(url->path)[2], "repos");
        a.expect(!url->query.has_value());
        a.expect(!url->fragment.has_value());

        a.expect_eq(url->serialize(), "file:///home/zero-one/repos/");
    });

    s.add_test("URL parsing: file URL with single-dot", [](etest::IActions &a) {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("file:///home/zero-one/repos/./hastur/README.md");

        a.require(url.has_value());

        a.expect_eq(url->scheme, "file");
        a.expect_eq(std::get<0>(url->host->data), "");
        a.expect(!url->port.has_value());
        a.expect_eq(std::get<1>(url->path)[0], "home");
        a.expect_eq(std::get<1>(url->path)[1], "zero-one");
        a.expect_eq(std::get<1>(url->path)[2], "repos");
        a.expect_eq(std::get<1>(url->path)[3], "hastur");
        a.expect_eq(std::get<1>(url->path)[4], "README.md");
        a.expect(!url->query.has_value());
        a.expect(!url->fragment.has_value());

        a.expect_eq(url->serialize(), "file:///home/zero-one/repos/hastur/README.md");
    });

    s.add_test("URL parsing: file URL with windows path", [](etest::IActions &a) {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse(R"(file://C:\Users\zero-one\repos\hastur\README.md)");

        a.require(url.has_value());

        a.expect_eq(url->scheme, "file");
        a.expect_eq(std::get<0>(url->host->data), "");
        a.expect(!url->port.has_value());
        a.expect_eq(std::get<1>(url->path)[0], "C:");
        a.expect_eq(std::get<1>(url->path)[1], "Users");
        a.expect_eq(std::get<1>(url->path)[2], "zero-one");
        a.expect_eq(std::get<1>(url->path)[3], "repos");
        a.expect_eq(std::get<1>(url->path)[4], "hastur");
        a.expect_eq(std::get<1>(url->path)[5], "README.md");
        a.expect(!url->query.has_value());
        a.expect(!url->fragment.has_value());

        a.expect_eq(url->serialize(), R"(file:///C:/Users/zero-one/repos/hastur/README.md)");
    });

    s.add_test("URL origin", [](etest::IActions &a) {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("https://example.com:8080/index.html");
        std::optional<url::Url> url2 = p.parse("https://example.com:9999/index.php");
        std::optional<url::Url> url3 = p.parse("http://example.com:8080/index.html");
        std::optional<url::Url> url4 = p.parse("https://example.com:8080/index.php?foo=bar");

        a.require(url.has_value());
        a.require(url2.has_value());
        a.require(url3.has_value());
        a.require(url4.has_value());

        url::Origin o = url->origin();
        url::Origin o2 = url2->origin();
        url::Origin o3 = url3->origin();
        url::Origin o4 = url4->origin();
        url::Origin o5{"https", {url::HostType::DnsDomain, "example.com"}, std::uint16_t{8080}, "example.com"};

        a.require(o.port.has_value());
        a.require(o2.port.has_value());
        a.require(o3.port.has_value());
        a.require(o4.port.has_value());

        a.expect(!o.domain.has_value());
        a.expect(!o2.domain.has_value());
        a.expect(!o3.domain.has_value());
        a.expect(!o4.domain.has_value());

        a.expect_eq(o.scheme, "https");
        a.expect_eq(o2.scheme, "https");
        a.expect_eq(o3.scheme, "http");
        a.expect_eq(o4.scheme, "https");

        a.expect_eq(o.host.serialize(), "example.com");
        a.expect_eq(o2.host.serialize(), "example.com");
        a.expect_eq(o3.host.serialize(), "example.com");
        a.expect_eq(o4.host.serialize(), "example.com");

        a.expect_eq(*o.port, 8080);
        a.expect_eq(*o2.port, 9999);
        a.expect_eq(*o3.port, 8080);
        a.expect_eq(*o4.port, 8080);

        a.expect(!o.opaque);
        a.expect(!o2.opaque);
        a.expect(!o3.opaque);
        a.expect(!o4.opaque);

        a.expect_eq(o.serialize(), "https://example.com:8080");
        a.expect_eq(o2.serialize(), "https://example.com:9999");
        a.expect_eq(o3.serialize(), "http://example.com:8080");
        a.expect_eq(o4.serialize(), "https://example.com:8080");

        a.expect(o != o2);
        a.expect(o != o3);
        a.expect(o == o4);
        a.expect(o == o5);

        a.expect(!o.is_same_origin_domain(o2));
        a.expect(!o.is_same_origin_domain(o3));
        a.expect(o.is_same_origin_domain(o4));
        a.expect(!o.is_same_origin_domain(o5));

        a.expect(std::holds_alternative<url::Host>(o.effective_domain()));
        a.expect(std::holds_alternative<url::Host>(o2.effective_domain()));
        a.expect(std::holds_alternative<url::Host>(o3.effective_domain()));
        a.expect(std::holds_alternative<url::Host>(o4.effective_domain()));
        a.expect(std::holds_alternative<std::string>(o5.effective_domain()));

        a.expect_eq(std::get<std::string>(o5.effective_domain()), "example.com");
    });

    s.add_test("URL origin: opaque origin", [](etest::IActions &a) {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("file:///usr/local/bin/foo");
        std::optional<url::Url> url2 = p.parse("file:///etc/passwd");
        std::optional<url::Url> url3 = p.parse("http://example.com");

        a.require(url.has_value());
        a.require(url2.has_value());
        a.require(url3.has_value());

        url::Origin o = url->origin();
        url::Origin o2 = url2->origin();
        url::Origin o3 = url3->origin();

        a.expect(o.opaque);
        a.expect(o2.opaque);
        a.expect(!o3.opaque);

        a.expect_eq(o.serialize(), "null");
        a.expect_eq(o2.serialize(), "null");

        a.expect(std::holds_alternative<std::monostate>(o.effective_domain()));
        a.expect(std::holds_alternative<std::monostate>(o2.effective_domain()));

        a.expect(o == o2);
        a.expect(o != o3);

        a.expect(o.is_same_origin_domain(o2));
        a.expect(!o.is_same_origin_domain(o3));
    });

    s.add_test("URL origin: blob URL", [](etest::IActions &a) {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("blob:https://whatwg.org/d0360e2f-caee-469f-9a2f-87d5b0456f6f");
        std::optional<url::Url> url2 = p.parse("blob:ws://whatwg.org/d0360e2f-caee-469f-9a2f-87d5b0456f6f");

        a.require(url.has_value());
        a.require(url2.has_value());

        url::Origin o = url->origin();
        url::Origin o2 = url2->origin();

        a.expect(!o.opaque);
        a.expect(o2.opaque);

        a.expect(!o.port.has_value());
        a.expect(!o.domain.has_value());

        a.expect_eq(o.scheme, "https");
        a.expect_eq(o.host.serialize(), "whatwg.org");

        a.expect_eq(o.serialize(), "https://whatwg.org");
        a.expect_eq(o2.serialize(), "null");
    });

    s.add_test("URL parsing: parse_host w/ empty input", [](etest::IActions &a) {
        url::UrlParser p;
        auto url = p.parse("a://");

        a.require(url.has_value());
        a.expect_eq(*url, url::Url{.scheme = "a", .host = url::Host{.type = url::HostType::Opaque}});
    });

    s.add_test("URL parsing: invalid utf-8", [](etest::IActions &) {
        url::UrlParser p;
        std::ignore = p.parse("\x6f\x3a\x2f\x2f\x26\xe1\xd2\x2e\x3b\xf5\x26\xe1\xd2\x0b\x0a\x26\xe1\xd2\xc9");
    });

    s.add_test("URL parsing: file url with base", [](etest::IActions &a) {
        url::UrlParser p;

        std::optional<url::Url> file_base = p.parse("file:///usr/bin/vim");

        a.require(file_base.has_value());

        std::optional<url::Url> url = p.parse("file:usr/bin/emacs", file_base);

        a.require(url.has_value());

        a.expect_eq(url->scheme, "file");
        a.expect_eq(url->serialize(), "file:///usr/bin/usr/bin/emacs");
        a.expect_eq(url->host->serialize(), "");
        a.expect_eq(url->serialize_path(), "/usr/bin/usr/bin/emacs");
    });

    s.add_test("URL parsing: file url backslash with base", [](etest::IActions &a) {
        url::UrlParser p;

        std::optional<url::Url> file_base = p.parse("file:///usr/bin/vim");

        a.require(file_base.has_value());

        std::optional<url::Url> url = p.parse("file:\\usr/bin/emacs", file_base);

        a.require(url.has_value());

        a.expect_eq(url->scheme, "file");
        a.expect_eq(url->serialize(), "file:///usr/bin/emacs");
        a.expect_eq(url->host->serialize(), "");
        a.expect_eq(url->serialize_path(), "/usr/bin/emacs");
    });

    s.add_test("URL parsing: non-relative url w/o scheme", [](etest::IActions &a) {
        auto [url, errors] = parse_url("//example.com");
        a.expect_eq(url, std::nullopt);
        a.expect_eq(errors, std::vector{url::ValidationError::MissingSchemeNonRelativeUrl});
    });

    s.add_test("URL normalization: uppercasing percent-encoded triplets", [](etest::IActions &a) {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("http://example.com/foo%2a");

        a.require(url.has_value());

        a.expect_eq(url->serialize(false, true), "http://example.com/foo%2A");
    });

    s.add_test("URL normalization: lowercasing scheme and host", [](etest::IActions &a) {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("HTTP://User@Example.COM/Foo");

        a.require(url.has_value());

        a.expect_eq(url->serialize(), "http://User@example.com/Foo");
    });

    s.add_test("URL normalization: decoding percent-encoded triplets of unreserved characters", [](etest::IActions &a) {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("http://example.com/%7Efoo");

        a.require(url.has_value());

        a.expect_eq(url->serialize(false, true), "http://example.com/~foo");
    });

    s.add_test("URL normalization: removing dot-segments", [](etest::IActions &a) {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("http://example.com/foo/./bar/baz/../qux");

        a.require(url.has_value());

        a.expect_eq(url->serialize(), "http://example.com/foo/bar/qux");
    });

    s.add_test("URL normalization: converting empty path to '/'", [](etest::IActions &a) {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("http://example.com");

        a.require(url.has_value());

        a.expect_eq(url->serialize(), "http://example.com/");
    });

    s.add_test("URL normalization: removing default port", [](etest::IActions &a) {
        url::UrlParser p;

        std::optional<url::Url> url = p.parse("http://example.com:80/");

        a.require(url.has_value());

        a.expect_eq(url->serialize(), "http://example.com/");
    });

    // NOLINTBEGIN(misc-include-cleaner): What you're meant to include from
    // simdjson depends on things like the architecture you're compiling for.
    // This is handled automagically with detection macros inside simdjson.
    s.add_test("Web Platform Tests", [](etest::IActions &a) {
        url::UrlParser p;

        simdjson::ondemand::parser parser;

        // NOLINTNEXTLINE(clang-analyzer-unix.Errno): Problem in simdjson that probably doesn't affect us.
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
                    a.expect(base_test.has_value(), "Parsing base URL:(" + std::string{base_str} + ") failed");

                    continue;
                }
            }

            // Parse input URL
            std::optional<url::Url> url = p.parse(std::string{input}, base_test);

            if (!should_fail) {
                a.expect(url.has_value(), "Parsing input URL:(" + std::string{input} + ") failed");

                if (!url.has_value()) {
                    continue;
                }
            } else {
                a.require(!url.has_value(),
                        "Parsing input URL:(" + std::string{input} + ") succeeded when it was supposed to fail");

                // If this test was an expected failure, test ends here
                continue;
            }

            // Check URL fields against test

            std::string_view href = obj["href"];
            a.expect_eq(url->serialize(), href);

            if (obj.find_field("failure").error() != simdjson::error_code::NO_SUCH_FIELD) {
                std::string_view origin = obj["origin"];

                a.expect_eq(url->origin().serialize(), origin);
            }

            std::string_view protocol = obj["protocol"];
            a.expect_eq(url->scheme + ":", protocol);

            std::string_view username = obj["username"];
            a.expect_eq(url->user, username);

            std::string_view password = obj["password"];
            a.expect_eq(url->passwd, password);

            std::string_view hostname = obj["hostname"];
            a.expect_eq(url->host.has_value() ? url->host->serialize() : "", hostname);

            std::string_view host = obj["host"];
            std::string host_serialized = url->host.has_value() ? url->host->serialize() : "";
            std::string host_port = url->port.has_value() ? std::string{":"} + std::to_string(*url->port) : "";
            a.expect_eq(host_serialized + host_port, host);

            std::string_view port = obj["port"];
            a.expect_eq(url->port.has_value() ? std::to_string(*url->port) : "", port);

            std::string_view pathname = obj["pathname"];
            a.expect_eq(url->serialize_path(), pathname);

            std::string_view search = obj["search"];
            a.expect_eq(url->query.has_value() && !url->query->empty() ? std::string{"?"} + *url->query : "", search);

            std::string_view hash = obj["hash"];
            a.expect_eq(url->fragment.has_value() && !url->fragment->empty() ? std::string{"#"} + *url->fragment : "",
                    hash);
        }
    });
    // NOLINTEND(misc-include-cleaner)

    int ret = s.run();

    url::icu_cleanup();

    return ret;
}
