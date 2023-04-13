// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "engine/engine.h"

#include "etest/etest.h"
#include "protocol/iprotocol_handler.h"
#include "protocol/response.h"
#include "uri/uri.h"

#include <map>
#include <string>
#include <utility>

using namespace std::literals;
using etest::expect;
using etest::expect_eq;
using etest::require;
using protocol::Error;
using protocol::Response;

namespace {

class FakeProtocolHandler final : public protocol::IProtocolHandler {
public:
    explicit FakeProtocolHandler(std::map<std::string, Response> responses) : responses_{std::move(responses)} {}
    [[nodiscard]] Response handle(uri::Uri const &uri) override { return responses_.at(uri.uri); }

private:
    std::map<std::string, Response> responses_;
};

bool contains(std::vector<css::Rule> const &stylesheet, css::Rule const &rule) {
    return std::ranges::find(stylesheet, rule) != end(stylesheet);
}

} // namespace

int main() {
    etest::test("no handlers set", [] {
        engine::Engine e{std::make_unique<FakeProtocolHandler>(std::map{
                std::pair{"hax://example.com"s, Response{.err = Error::Unresolved}},
        })};
        e.navigate(uri::Uri::parse("hax://example.com"));

        e = engine::Engine{std::make_unique<FakeProtocolHandler>(std::map{
                std::pair{"hax://example.com"s, Response{.err = Error::Ok}},
        })};
        e.navigate(uri::Uri::parse("hax://example.com"));
        e.set_layout_width(10);
    });

    etest::test("css in <head><style>", [] {
        std::map<std::string, Response> responses{{
                "hax://example.com"s,
                Response{
                        .err = Error::Ok,
                        .status_line = {.status_code = 200},
                        .body{"<html><head><style>p { font-size: 123em; }</style></head></html>"},
                },
        }};
        engine::Engine e{std::make_unique<FakeProtocolHandler>(std::move(responses))};
        e.navigate(uri::Uri::parse("hax://example.com"));
        expect_eq(e.stylesheet().back(),
                css::Rule{
                        .selectors{"p"},
                        .declarations{{css::PropertyId::FontSize, "123em"}},
                });
    });

    etest::test("navigation failure", [] {
        bool success{false};
        engine::Engine e{std::make_unique<FakeProtocolHandler>(std::map{
                std::pair{"hax://example.com"s, Response{.err = Error::Unresolved}},
        })};
        e.set_on_navigation_failure([&](Error err) { success = err != Error::Ok; });
        e.set_on_page_loaded([] { require(false); });
        e.set_on_layout_updated([] { require(false); });

        e.navigate(uri::Uri::parse("hax://example.com"));
        expect(success);
    });

    etest::test("page load", [] {
        bool success{false};
        engine::Engine e{std::make_unique<FakeProtocolHandler>(std::map{
                std::pair{"hax://example.com"s, Response{.err = Error::Ok}},
        })};
        e.set_on_navigation_failure([&](Error) { require(false); });
        e.set_on_page_loaded([&] { success = true; });
        e.set_on_layout_updated([] { require(false); });

        e.navigate(uri::Uri::parse("hax://example.com"));
        expect(success);
    });

    etest::test("layout update", [] {
        engine::Engine e{std::make_unique<FakeProtocolHandler>(std::map{
                std::pair{"hax://example.com"s, Response{.err = Error::Ok}},
        })};
        e.set_on_navigation_failure([&](Error) { require(false); });
        e.set_on_page_loaded([] { require(false); });
        e.set_on_layout_updated([] { require(false); });

        e.set_layout_width(10);

        bool success{false};
        e.set_on_page_loaded([&] { success = true; });

        e.navigate(uri::Uri::parse("hax://example.com"));

        expect(success);

        e.set_on_page_loaded([&] { require(false); });
        success = false;
        e.set_on_layout_updated([&] { success = true; });

        e.set_layout_width(100);
        expect(success);
    });

    etest::test("css in <head><style> takes priority over browser built-in css", [] {
        std::map<std::string, Response> responses{{
                "hax://example.com"s,
                Response{
                        .err = Error::Ok,
                        .status_line = {.status_code = 200},
                        .body{"<html></html>"},
                },
        }};
        engine::Engine e{std::make_unique<FakeProtocolHandler>(std::move(responses))};
        e.navigate(uri::Uri::parse("hax://example.com"));
        // Our default CSS gives <html> the property display: block.
        require(e.layout());
        expect_eq(e.layout()->get_property<css::PropertyId::Display>(), style::DisplayValue::Block);

        responses = std::map<std::string, Response>{{
                "hax://example.com"s,
                Response{
                        .err = Error::Ok,
                        .status_line = {.status_code = 200},
                        .body{"<html><head><style>html { display: inline; }</style></head></html>"},
                },
        }};

        e = engine::Engine{std::make_unique<FakeProtocolHandler>(std::move(responses))};
        e.navigate(uri::Uri::parse("hax://example.com"));

        // The CSS declared in the page should have a higher priority and give
        // <html> the property display: inline.
        require(e.layout());
        expect_eq(e.layout()->get_property<css::PropertyId::Display>(), style::DisplayValue::Inline);
    });

    etest::test("stylesheet link, parallel download", [] {
        std::map<std::string, Response> responses;
        responses["hax://example.com"s] = Response{
                .err = Error::Ok,
                .status_line = {.status_code = 200},
                .body{"<html><head>"
                      "<link rel=stylesheet href=one.css />"
                      "<link rel=stylesheet href=two.css />"
                      "</head></html>"},
        };
        responses["hax://example.com/one.css"s] = Response{
                .err = Error::Ok,
                .status_line = {.status_code = 200},
                .body{"p { font-size: 123em; }"},
        };
        responses["hax://example.com/two.css"s] = Response{
                .err = Error::Ok,
                .status_line = {.status_code = 200},
                .body{"p { color: green; }"},
        };
        engine::Engine e{std::make_unique<FakeProtocolHandler>(std::move(responses))};
        e.navigate(uri::Uri::parse("hax://example.com"));
        expect(contains(e.stylesheet(), {.selectors{"p"}, .declarations{{css::PropertyId::FontSize, "123em"}}}));
        expect(contains(e.stylesheet(), {.selectors{"p"}, .declarations{{css::PropertyId::Color, "green"}}}));
    });

    etest::test("stylesheet link, unsupported Content-Encoding", [] {
        std::map<std::string, Response> responses;
        responses["hax://example.com"s] = Response{
                .err = Error::Ok,
                .status_line = {.status_code = 200},
                .body{"<html><head><link rel=stylesheet href=lol.css /></head></html>"},
        };
        responses["hax://example.com/lol.css"s] = Response{
                .err = Error::Ok,
                .status_line = {.status_code = 200},
                .headers{{"Content-Encoding", "really-borked-content-type"}},
                .body{"p { font-size: 123em; }"},
        };
        engine::Engine e{std::make_unique<FakeProtocolHandler>(std::move(responses))};
        e.navigate(uri::Uri::parse("hax://example.com"));
        expect(!contains(e.stylesheet(), {.selectors{"p"}, .declarations{{css::PropertyId::FontSize, "123em"}}}));
    });

    // p { font-size: 123em; }, gzipped.
    std::string gzipped_css =
            "\x1f\x8b\x08\x00\x00\x00\x00\x00\x00\x03\x2b\x50\xa8\x56\x48\xcb\xcf\x2b\xd1\x2d\xce\xac\x4a\xb5\x52\x30\x34\x32\x4e\xcd\xb5\x56\xa8\xe5\x02\x00\x0c\x97\x72\x35\x18\x00\x00\x00"s;

    etest::test("stylesheet link, gzip Content-Encoding", [gzipped_css] {
        std::map<std::string, Response> responses;
        responses["hax://example.com"s] = Response{
                .err = Error::Ok,
                .status_line = {.status_code = 200},
                .body{"<html><head><link rel=stylesheet href=lol.css /></head></html>"},
        };
        responses["hax://example.com/lol.css"s] = Response{
                .err = Error::Ok,
                .status_line = {.status_code = 200},
                .headers{{"Content-Encoding", "gzip"}},
                .body{gzipped_css},
        };
        engine::Engine e{std::make_unique<FakeProtocolHandler>(responses)};
        e.navigate(uri::Uri::parse("hax://example.com"));
        expect(std::ranges::find(e.stylesheet(),
                       css::Rule{
                               .selectors{"p"},
                               .declarations{{css::PropertyId::FontSize, "123em"}},
                       })
                != end(e.stylesheet()));

        // And again, but with x-gzip instead.
        responses["hax://example.com/lol.css"s] = Response{
                .err = Error::Ok,
                .status_line = {.status_code = 200},
                .headers{{"Content-Encoding", "x-gzip"}},
                .body{std::move(gzipped_css)},
        };
        e = engine::Engine{std::make_unique<FakeProtocolHandler>(responses)};
        e.navigate(uri::Uri::parse("hax://example.com"));
        expect(std::ranges::find(e.stylesheet(),
                       css::Rule{
                               .selectors{"p"},
                               .declarations{{css::PropertyId::FontSize, "123em"}},
                       })
                != end(e.stylesheet()));
    });

    etest::test("stylesheet link, gzip Content-Encoding, bad header", [gzipped_css]() mutable {
        std::map<std::string, Response> responses;
        responses["hax://example.com"s] = Response{
                .err = Error::Ok,
                .status_line = {.status_code = 200},
                .body{"<html><head><link rel=stylesheet href=lol.css /></head></html>"},
        };
        // Ruin the gzip header.
        gzipped_css[1] += 1;
        responses["hax://example.com/lol.css"s] = Response{
                .err = Error::Ok,
                .status_line = {.status_code = 200},
                .headers{{"Content-Encoding", "gzip"}},
                .body{std::move(gzipped_css)},
        };
        engine::Engine e{std::make_unique<FakeProtocolHandler>(std::move(responses))};
        e.navigate(uri::Uri::parse("hax://example.com"));
        expect(std::ranges::find(e.stylesheet(),
                       css::Rule{
                               .selectors{"p"},
                               .declarations{{css::PropertyId::FontSize, "123em"}},
                       })
                == end(e.stylesheet()));
    });

    etest::test("stylesheet link, gzip Content-Encoding, crc32 mismatch", [gzipped_css]() mutable {
        std::map<std::string, Response> responses;
        responses["hax://example.com"s] = Response{
                .err = Error::Ok,
                .status_line = {.status_code = 200},
                .body{"<html><head><link rel=stylesheet href=lol.css /></head></html>"},
        };
        // Ruin the content.
        gzipped_css[20] += 1;
        responses["hax://example.com/lol.css"s] = Response{
                .err = Error::Ok,
                .status_line = {.status_code = 200},
                .headers{{"Content-Encoding", "gzip"}},
                .body{std::move(gzipped_css)},
        };
        engine::Engine e{std::make_unique<FakeProtocolHandler>(std::move(responses))};
        e.navigate(uri::Uri::parse("hax://example.com"));
        expect(std::ranges::find(e.stylesheet(),
                       css::Rule{
                               .selectors{"p"},
                               .declarations{{css::PropertyId::FontSize, "123em"}},
                       })
                == end(e.stylesheet()));
    });

    return etest::run_all_tests();
}
