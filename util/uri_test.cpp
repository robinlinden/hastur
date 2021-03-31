#include "util/uri.h"

#include "etest/etest.h"

using etest::expect;
using util::Uri;

int main() {
    etest::test("https: user, pass, port, path, query", [] {
        auto https_uri = *Uri::parse("https://zero-one:muh_password@example-domain.net:8080/muh/long/path.html?foo=bar");

        expect(https_uri.scheme == "https");
        expect(https_uri.authority.user == "zero-one");
        expect(https_uri.authority.passwd == "muh_password");
        expect(https_uri.authority.host == "example-domain.net");
        expect(https_uri.authority.port == "8080");
        expect(https_uri.path == "/muh/long/path.html");
        expect(https_uri.query == "foo=bar");
        expect(https_uri.fragment == "");
    });

    etest::test("https: user, pass, path, query", [] {
        auto https_uri = *Uri::parse("https://zero-one:muh_password@example-domain.net/muh/long/path.html?foo=bar");

        expect(https_uri.scheme == "https");
        expect(https_uri.authority.user == "zero-one");
        expect(https_uri.authority.passwd == "muh_password");
        expect(https_uri.authority.host == "example-domain.net");
        expect(https_uri.authority.port == "");
        expect(https_uri.path == "/muh/long/path.html");
        expect(https_uri.query == "foo=bar");
        expect(https_uri.fragment == "");
    });

    etest::test("https: user, path, query", [] {
        auto https_uri = *Uri::parse("https://zero-one@example-domain.net/muh/long/path.html?foo=bar");

        expect(https_uri.scheme == "https");
        expect(https_uri.authority.user == "zero-one");
        expect(https_uri.authority.passwd == "");
        expect(https_uri.authority.host == "example-domain.net");
        expect(https_uri.authority.port == "");
        expect(https_uri.path == "/muh/long/path.html");
        expect(https_uri.query == "foo=bar");
        expect(https_uri.fragment == "");
    });

    etest::test("https: path, query", [] {
        auto https_uri = *Uri::parse("https://example-domain.net/muh/long/path.html?foo=bar");

        expect(https_uri.scheme == "https");
        expect(https_uri.authority.user == "");
        expect(https_uri.authority.passwd == "");
        expect(https_uri.authority.host == "example-domain.net");
        expect(https_uri.authority.port == "");
        expect(https_uri.path == "/muh/long/path.html");
        expect(https_uri.query == "foo=bar");
        expect(https_uri.fragment == "");
    });

    etest::test("https: path, fragment", [] {
        auto https_uri = *Uri::parse("https://example-domain.net/muh/long/path.html#About");

        expect(https_uri.scheme == "https");
        expect(https_uri.authority.user == "");
        expect(https_uri.authority.passwd == "");
        expect(https_uri.authority.host == "example-domain.net");
        expect(https_uri.authority.port == "");
        expect(https_uri.path == "/muh/long/path.html");
        expect(https_uri.query == "");
        expect(https_uri.fragment == "About");
    });

    etest::test("mailto: path", [] {
        auto mailto_uri = *Uri::parse("mailto:example@example.net");

        expect(mailto_uri.scheme == "mailto");
        expect(mailto_uri.authority.user == "");
        expect(mailto_uri.authority.passwd == "");
        expect(mailto_uri.authority.host == "");
        expect(mailto_uri.authority.port == "");
        expect(mailto_uri.path == "example@example.net");
        expect(mailto_uri.query == "");
        expect(mailto_uri.fragment == "");
    });

    etest::test("tel: path", [] {
        auto tel_uri = *Uri::parse("tel:+1-830-476-5664");

        expect(tel_uri.scheme == "tel");
        expect(tel_uri.authority.user == "");
        expect(tel_uri.authority.passwd == "");
        expect(tel_uri.authority.host == "");
        expect(tel_uri.authority.port == "");
        expect(tel_uri.path == "+1-830-476-5664");
        expect(tel_uri.query == "");
        expect(tel_uri.fragment == "");
    });

    // TODO(Zer0-One): Test for parsing failure.
    // etest::test("parse failure", [] {
    //     auto tel_uri = Uri::parse("");
    //     expect(!tel_uri.has_value());
    // });

    return etest::run_all_tests();
}
