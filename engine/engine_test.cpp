// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "engine/engine.h"

#include "css/property_id.h"
#include "css/rule.h"
#include "dom/dom.h"
#include "etest/etest.h"
#include "gfx/color.h"
#include "protocol/iprotocol_handler.h"
#include "protocol/response.h"
#include "style/styled_node.h"
#include "type/naive.h"
#include "type/type.h"
#include "uri/uri.h"

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

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
        expect_eq(e.stylesheet().rules.back(),
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

    etest::test("multiple inline <head><style> elements are allowed", [] {
        std::map<std::string, Response> responses{{
                "hax://example.com"s,
                Response{
                        .err = Error::Ok,
                        .status_line = {.status_code = 200},
                        .body{"<html><head><style>"
                              "a { color: red; } "
                              "p { color: green; }"
                              "</style>"
                              "<style></style>"
                              "<style>p { color: cyan; }</style>"
                              "<p><a>"},
                },
        }};
        engine::Engine e{std::make_unique<FakeProtocolHandler>(std::move(responses))};
        e.navigate(uri::Uri::parse("hax://example.com"));
        require(e.layout());
        auto const *a = dom::nodes_by_xpath(*e.layout(), "//a"sv).at(0);
        expect_eq(a->get_property<css::PropertyId::Color>(), gfx::Color::from_css_name("red"));
        auto const *p = dom::nodes_by_xpath(*e.layout(), "//p"sv).at(0);
        expect_eq(p->get_property<css::PropertyId::Color>(), gfx::Color::from_css_name("cyan"));
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
        expect(contains(e.stylesheet().rules, {.selectors{"p"}, .declarations{{css::PropertyId::FontSize, "123em"}}}));
        expect(contains(e.stylesheet().rules, {.selectors{"p"}, .declarations{{css::PropertyId::Color, "green"}}}));
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
        expect(!contains(e.stylesheet().rules, {.selectors{"p"}, .declarations{{css::PropertyId::FontSize, "123em"}}}));
    });

    // p { font-size: 123em; }, gzipped.
    std::string gzipped_css =
            "\x1f\x8b\x08\x00\x00\x00\x00\x00\x00\x03\x2b\x50\xa8\x56\x48\xcb\xcf\x2b\xd1\x2d\xce\xac\x4a\xb5\x52\x30\x34\x32\x4e\xcd\xb5\x56\xa8\xe5\x02\x00\x0c\x97\x72\x35\x18\x00\x00\x00"s;

    std::string zlibbed_css =
            "\x78\x5e\x2b\x50\xa8\x56\x48\xcb\xcf\x2b\xd1\x2d\xce\xac\x4a\xb5\x52\x30\x34\x32\x4e\xcd\xb5\x56\xa8\xe5\x02\x00\x63\xc3\x07\x6f"s;

    etest::test("stylesheet link, gzip Content-Encoding", [gzipped_css]() mutable {
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
        expect(std::ranges::find(e.stylesheet().rules,
                       css::Rule{
                               .selectors{"p"},
                               .declarations{{css::PropertyId::FontSize, "123em"}},
                       })
                != end(e.stylesheet().rules));

        // And again, but with x-gzip instead.
        responses["hax://example.com/lol.css"s] = Response{
                .err = Error::Ok,
                .status_line = {.status_code = 200},
                .headers{{"Content-Encoding", "x-gzip"}},
                .body{std::move(gzipped_css)},
        };
        e = engine::Engine{std::make_unique<FakeProtocolHandler>(responses)};
        e.navigate(uri::Uri::parse("hax://example.com"));
        expect(std::ranges::find(e.stylesheet().rules,
                       css::Rule{
                               .selectors{"p"},
                               .declarations{{css::PropertyId::FontSize, "123em"}},
                       })
                != end(e.stylesheet().rules));
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
        expect(std::ranges::find(e.stylesheet().rules,
                       css::Rule{
                               .selectors{"p"},
                               .declarations{{css::PropertyId::FontSize, "123em"}},
                       })
                == end(e.stylesheet().rules));
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
        expect(std::ranges::find(e.stylesheet().rules,
                       css::Rule{
                               .selectors{"p"},
                               .declarations{{css::PropertyId::FontSize, "123em"}},
                       })
                == end(e.stylesheet().rules));
    });

    etest::test("stylesheet link, gzip Content-Encoding, served zlib", [zlibbed_css] {
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
                .body{zlibbed_css},
        };
        engine::Engine e{std::make_unique<FakeProtocolHandler>(responses)};
        e.navigate(uri::Uri::parse("hax://example.com"));
        expect(std::ranges::find(e.stylesheet().rules,
                       css::Rule{
                               .selectors{"p"},
                               .declarations{{css::PropertyId::FontSize, "123em"}},
                       })
                == end(e.stylesheet().rules));
    });

    etest::test("stylesheet link, deflate Content-Encoding", [zlibbed_css] {
        std::map<std::string, Response> responses;
        responses["hax://example.com"s] = Response{
                .err = Error::Ok,
                .status_line = {.status_code = 200},
                .body{"<html><head><link rel=stylesheet href=lol.css /></head></html>"},
        };
        responses["hax://example.com/lol.css"s] = Response{
                .err = Error::Ok,
                .status_line = {.status_code = 200},
                .headers{{"Content-Encoding", "deflate"}},
                .body{zlibbed_css},
        };
        engine::Engine e{std::make_unique<FakeProtocolHandler>(responses)};
        e.navigate(uri::Uri::parse("hax://example.com"));
        expect(std::ranges::find(e.stylesheet().rules,
                       css::Rule{
                               .selectors{"p"},
                               .declarations{{css::PropertyId::FontSize, "123em"}},
                       })
                != end(e.stylesheet().rules));
    });

    etest::test("redirect", [] {
        std::map<std::string, Response> responses;
        responses["hax://example.com"s] = Response{
                .err = Error::Ok,
                .status_line = {.status_code = 301},
                .headers = {{"Location", "hax://example.com/redirected"}},
        };
        responses["hax://example.com/redirected"s] = Response{
                .err = Error::Ok,
                .status_line = {.status_code = 200},
                .body{"<html><body>hello!</body></html>"},
        };
        engine::Engine e{std::make_unique<FakeProtocolHandler>(std::move(responses))};
        expect_eq(e.navigate(uri::Uri::parse("hax://example.com")), protocol::Error::Ok);
        expect_eq(e.uri().uri, "hax://example.com/redirected");

        auto const &body = std::get<dom::Element>(e.dom().html().children.at(1));
        expect_eq(std::get<dom::Text>(body.children.at(0)).text, "hello!"sv);
    });

    etest::test("redirect not providing Location header", [] {
        std::map<std::string, Response> responses;
        responses["hax://example.com"s] = Response{
                .err = Error::Ok,
                .status_line = {.status_code = 301},
        };
        engine::Engine e{std::make_unique<FakeProtocolHandler>(std::move(responses))};
        expect_eq(e.navigate(uri::Uri::parse("hax://example.com")), protocol::Error::InvalidResponse);
    });

    etest::test("redirect, style", [] {
        std::map<std::string, Response> responses;
        responses["hax://example.com"s] = Response{
                .err = Error::Ok,
                .status_line = {.status_code = 200},
                .body{"<html><head>"
                      "<link rel=stylesheet href=hello.css />"
                      "</head></html>"},
        };
        responses["hax://example.com/hello.css"s] = Response{
                .err = Error::Ok,
                .status_line = {.status_code = 301},
                .headers = {{"Location", "hax://example.com/redirected.css"}},
        };
        responses["hax://example.com/redirected.css"s] = Response{
                .err = Error::Ok,
                .status_line = {.status_code = 200},
                .body{"p { color: green; }"},
        };
        engine::Engine e{std::make_unique<FakeProtocolHandler>(std::move(responses))};
        expect_eq(e.navigate(uri::Uri::parse("hax://example.com")), protocol::Error::Ok);
        expect(contains(e.stylesheet().rules, {.selectors{"p"}, .declarations{{css::PropertyId::Color, "green"}}}));
    });

    etest::test("redirect loop", [] {
        std::map<std::string, Response> responses;
        responses["hax://example.com"s] = Response{
                .err = Error::Ok,
                .status_line = {.status_code = 301},
                .headers = {{"Location", "hax://example.com"}},
        };
        engine::Engine e{std::make_unique<FakeProtocolHandler>(std::move(responses))};
        expect_eq(e.navigate(uri::Uri::parse("hax://example.com")), protocol::Error::RedirectLimit);
    });

    etest::test("load", [] {
        std::map<std::string, Response> responses;
        responses["hax://example.com"s] = Response{
                .err = Error::Ok,
                .status_line = {.status_code = 301},
                .headers = {{"Location", "hax://example.com/redirected"}},
        };
        responses["hax://example.com/redirected"s] = Response{
                .err = Error::Ok,
                .status_line = {.status_code = 200},
                .body{"<html><body>hello!</body></html>"},
        };
        engine::Engine e{std::make_unique<FakeProtocolHandler>(responses)};
        auto res = e.load(uri::Uri::parse("hax://example.com"));
        expect_eq(res.uri_after_redirects, uri::Uri::parse("hax://example.com/redirected"));
        expect_eq(res.response, responses.at("hax://example.com/redirected"));
    });

    etest::test("IType accessor, you get what you give", [] {
        auto naive = std::make_unique<type::NaiveType>();
        type::IType const *saved = naive.get();
        engine::Engine e{std::make_unique<FakeProtocolHandler>(std::map<std::string, Response>{}), std::move(naive)};
        expect_eq(&e.font_system(), saved);
    });

    return etest::run_all_tests();
}
