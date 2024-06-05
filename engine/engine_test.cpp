// SPDX-FileCopyrightText: 2021-2024 Robin Lindén <dev@robinlinden.eu>
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

#include <tl/expected.hpp>

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
using protocol::ErrorCode;
using protocol::Response;

using Responses = std::map<std::string, tl::expected<Response, protocol::Error>>;

namespace {

class FakeProtocolHandler final : public protocol::IProtocolHandler {
public:
    explicit FakeProtocolHandler(Responses responses) : responses_{std::move(responses)} {}
    [[nodiscard]] tl::expected<Response, protocol::Error> handle(uri::Uri const &uri) override {
        return responses_.at(uri.uri);
    }

private:
    Responses responses_;
};

bool contains(std::vector<css::Rule> const &stylesheet, css::Rule const &rule) {
    return std::ranges::find(stylesheet, rule) != end(stylesheet);
}

} // namespace

int main() {
    etest::test("css in <head><style>", [] {
        Responses responses{{
                "hax://example.com"s,
                Response{
                        .status_line = {.status_code = 200},
                        .body{"<html><head><style>p { font-size: 123em; }</style></head></html>"},
                },
        }};
        engine::Engine e{std::make_unique<FakeProtocolHandler>(std::move(responses))};
        auto page = e.navigate(uri::Uri::parse("hax://example.com").value());
        expect_eq(page.value()->stylesheet.rules.back(),
                css::Rule{
                        .selectors{"p"},
                        .declarations{{css::PropertyId::FontSize, "123em"}},
                });
    });

    etest::test("navigation failure", [] {
        engine::Engine e{std::make_unique<FakeProtocolHandler>(Responses{
                std::pair{"hax://example.com"s, tl::unexpected{protocol::Error{ErrorCode::Unresolved}}},
        })};

        auto page = e.navigate(uri::Uri::parse("hax://example.com").value());
        expect_eq(page.has_value(), false);
    });

    etest::test("page load", [] {
        engine::Engine e{std::make_unique<FakeProtocolHandler>(Responses{
                std::pair{"hax://example.com"s, Response{}},
        })};

        auto page = e.navigate(uri::Uri::parse("hax://example.com").value());
        expect(page.has_value());
    });

    etest::test("layout update", [] {
        engine::Engine e{std::make_unique<FakeProtocolHandler>(Responses{
                std::pair{"hax://example.com"s, Response{}},
        })};

        auto page = e.navigate(uri::Uri::parse("hax://example.com").value(), {.layout_width = 123}).value();
        expect_eq(page->layout_width, 123);

        e.relayout(*page, {.layout_width = 100});
        expect_eq(page->layout_width, 100);
    });

    etest::test("css in <head><style> takes priority over browser built-in css", [] {
        Responses responses{{
                "hax://example.com"s,
                Response{
                        .status_line = {.status_code = 200},
                        .body{"<html></html>"},
                },
        }};
        engine::Engine e{std::make_unique<FakeProtocolHandler>(std::move(responses))};
        auto page = e.navigate(uri::Uri::parse("hax://example.com").value()).value();
        // Our default CSS gives <html> the property display: block.
        require(page->layout.has_value());
        expect_eq(page->layout->get_property<css::PropertyId::Display>(), style::DisplayValue::Block);

        responses = Responses{{
                "hax://example.com"s,
                Response{
                        .status_line = {.status_code = 200},
                        .body{"<html><head><style>html { display: inline; }</style></head></html>"},
                },
        }};

        e = engine::Engine{std::make_unique<FakeProtocolHandler>(std::move(responses))};
        page = e.navigate(uri::Uri::parse("hax://example.com").value()).value();

        // The CSS declared in the page should have a higher priority and give
        // <html> the property display: inline.
        require(page->layout.has_value());
        expect_eq(page->layout->get_property<css::PropertyId::Display>(), style::DisplayValue::Inline);
    });

    etest::test("multiple inline <head><style> elements are allowed", [] {
        Responses responses{{
                "hax://example.com"s,
                Response{
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
        auto page = e.navigate(uri::Uri::parse("hax://example.com").value()).value();
        require(page->layout.has_value());
        auto const *a = dom::nodes_by_xpath(*page->layout, "//a"sv).at(0);
        expect_eq(a->get_property<css::PropertyId::Color>(), gfx::Color::from_css_name("red"));
        auto const *p = dom::nodes_by_xpath(*page->layout, "//p"sv).at(0);
        expect_eq(p->get_property<css::PropertyId::Color>(), gfx::Color::from_css_name("cyan"));
    });

    etest::test("stylesheet link, parallel download", [] {
        Responses responses;
        responses["hax://example.com"s] = Response{
                .status_line = {.status_code = 200},
                .body{"<html><head>"
                      "<link rel=stylesheet href=one.css />"
                      "<link rel=stylesheet href=two.css />"
                      "</head></html>"},
        };
        responses["hax://example.com/one.css"s] = Response{
                .status_line = {.status_code = 200},
                .body{"p { font-size: 123em; }"},
        };
        responses["hax://example.com/two.css"s] = Response{
                .status_line = {.status_code = 200},
                .body{"p { color: green; }"},
        };
        engine::Engine e{std::make_unique<FakeProtocolHandler>(std::move(responses))};
        auto page = e.navigate(uri::Uri::parse("hax://example.com").value()).value();
        expect(contains(
                page->stylesheet.rules, {.selectors{"p"}, .declarations{{css::PropertyId::FontSize, "123em"}}}));
        expect(contains(page->stylesheet.rules, {.selectors{"p"}, .declarations{{css::PropertyId::Color, "green"}}}));
    });

    etest::test("stylesheet link, unsupported Content-Encoding", [] {
        Responses responses;
        responses["hax://example.com"s] = Response{
                .status_line = {.status_code = 200},
                .body{"<html><head><link rel=stylesheet href=lol.css /></head></html>"},
        };
        responses["hax://example.com/lol.css"s] = Response{
                .status_line = {.status_code = 200},
                .headers{{"Content-Encoding", "really-borked-content-type"}},
                .body{"p { font-size: 123em; }"},
        };
        engine::Engine e{std::make_unique<FakeProtocolHandler>(std::move(responses))};
        auto page = e.navigate(uri::Uri::parse("hax://example.com").value()).value();
        expect(!contains(
                page->stylesheet.rules, {.selectors{"p"}, .declarations{{css::PropertyId::FontSize, "123em"}}}));
    });

    // p { font-size: 123em; }, gzipped.
    std::string gzipped_css =
            "\x1f\x8b\x08\x00\x00\x00\x00\x00\x00\x03\x2b\x50\xa8\x56\x48\xcb\xcf\x2b\xd1\x2d\xce\xac\x4a\xb5\x52\x30\x34\x32\x4e\xcd\xb5\x56\xa8\xe5\x02\x00\x0c\x97\x72\x35\x18\x00\x00\x00"s;

    std::string zlibbed_css =
            "\x78\x5e\x2b\x50\xa8\x56\x48\xcb\xcf\x2b\xd1\x2d\xce\xac\x4a\xb5\x52\x30\x34\x32\x4e\xcd\xb5\x56\xa8\xe5\x02\x00\x63\xc3\x07\x6f"s;

    // zstd-compressed `p { font-size: 123em; }`, generated with:
    // echo -n "p { font-size: 123em; }" | zstd -19 -
    std::string zstd_compressed_css =
            "\x28\xb5\x2f\xfd\x04\x68\xb9\x00\x00\x70\x20\x7b\x20\x66\x6f\x6e\x74\x2d\x73\x69\x7a\x65\x3a\x20\x31\x32\x33\x65\x6d\x3b\x20\x7d\x32\x30\x89\x03"s;

    etest::test("stylesheet link, gzip Content-Encoding", [gzipped_css]() mutable {
        Responses responses;
        responses["hax://example.com"s] = Response{
                .status_line = {.status_code = 200},
                .body{"<html><head><link rel=stylesheet href=lol.css /></head></html>"},
        };
        responses["hax://example.com/lol.css"s] = Response{
                .status_line = {.status_code = 200},
                .headers{{"Content-Encoding", "gzip"}},
                .body{gzipped_css},
        };
        engine::Engine e{std::make_unique<FakeProtocolHandler>(responses)};
        auto page = e.navigate(uri::Uri::parse("hax://example.com").value()).value();
        expect(std::ranges::find(page->stylesheet.rules,
                       css::Rule{
                               .selectors{"p"},
                               .declarations{{css::PropertyId::FontSize, "123em"}},
                       })
                != end(page->stylesheet.rules));

        // And again, but with x-gzip instead.
        responses["hax://example.com/lol.css"s] = Response{
                .status_line = {.status_code = 200},
                .headers{{"Content-Encoding", "x-gzip"}},
                .body{std::move(gzipped_css)},
        };
        e = engine::Engine{std::make_unique<FakeProtocolHandler>(responses)};
        page = e.navigate(uri::Uri::parse("hax://example.com").value()).value();
        expect(std::ranges::find(page->stylesheet.rules,
                       css::Rule{
                               .selectors{"p"},
                               .declarations{{css::PropertyId::FontSize, "123em"}},
                       })
                != end(page->stylesheet.rules));
    });

    etest::test("stylesheet link, gzip Content-Encoding, bad header", [gzipped_css]() mutable {
        Responses responses;
        responses["hax://example.com"s] = Response{
                .status_line = {.status_code = 200},
                .body{"<html><head><link rel=stylesheet href=lol.css /></head></html>"},
        };
        // Ruin the gzip header.
        gzipped_css[1] += 1;
        responses["hax://example.com/lol.css"s] = Response{
                .status_line = {.status_code = 200},
                .headers{{"Content-Encoding", "gzip"}},
                .body{std::move(gzipped_css)},
        };
        engine::Engine e{std::make_unique<FakeProtocolHandler>(std::move(responses))};
        auto page = e.navigate(uri::Uri::parse("hax://example.com").value()).value();
        expect(std::ranges::find(page->stylesheet.rules,
                       css::Rule{
                               .selectors{"p"},
                               .declarations{{css::PropertyId::FontSize, "123em"}},
                       })
                == end(page->stylesheet.rules));
    });

    etest::test("stylesheet link, gzip Content-Encoding, crc32 mismatch", [gzipped_css]() mutable {
        Responses responses;
        responses["hax://example.com"s] = Response{
                .status_line = {.status_code = 200},
                .body{"<html><head><link rel=stylesheet href=lol.css /></head></html>"},
        };
        // Ruin the content.
        gzipped_css[20] += 1;
        responses["hax://example.com/lol.css"s] = Response{
                .status_line = {.status_code = 200},
                .headers{{"Content-Encoding", "gzip"}},
                .body{std::move(gzipped_css)},
        };
        engine::Engine e{std::make_unique<FakeProtocolHandler>(std::move(responses))};
        auto page = e.navigate(uri::Uri::parse("hax://example.com").value()).value();
        expect(std::ranges::find(page->stylesheet.rules,
                       css::Rule{
                               .selectors{"p"},
                               .declarations{{css::PropertyId::FontSize, "123em"}},
                       })
                == end(page->stylesheet.rules));
    });

    etest::test("stylesheet link, gzip Content-Encoding, served zlib", [zlibbed_css] {
        Responses responses;
        responses["hax://example.com"s] = Response{
                .status_line = {.status_code = 200},
                .body{"<html><head><link rel=stylesheet href=lol.css /></head></html>"},
        };
        responses["hax://example.com/lol.css"s] = Response{
                .status_line = {.status_code = 200},
                .headers{{"Content-Encoding", "gzip"}},
                .body{zlibbed_css},
        };
        engine::Engine e{std::make_unique<FakeProtocolHandler>(responses)};
        auto page = e.navigate(uri::Uri::parse("hax://example.com").value()).value();
        expect(std::ranges::find(page->stylesheet.rules,
                       css::Rule{
                               .selectors{"p"},
                               .declarations{{css::PropertyId::FontSize, "123em"}},
                       })
                == end(page->stylesheet.rules));
    });

    etest::test("stylesheet link, deflate Content-Encoding", [zlibbed_css] {
        Responses responses;
        responses["hax://example.com"s] = Response{
                .status_line = {.status_code = 200},
                .body{"<html><head><link rel=stylesheet href=lol.css /></head></html>"},
        };
        responses["hax://example.com/lol.css"s] = Response{
                .status_line = {.status_code = 200},
                .headers{{"Content-Encoding", "deflate"}},
                .body{zlibbed_css},
        };
        engine::Engine e{std::make_unique<FakeProtocolHandler>(responses)};
        auto page = e.navigate(uri::Uri::parse("hax://example.com").value()).value();
        expect(std::ranges::find(page->stylesheet.rules,
                       css::Rule{
                               .selectors{"p"},
                               .declarations{{css::PropertyId::FontSize, "123em"}},
                       })
                != end(page->stylesheet.rules));
    });

    etest::test("stylesheet link, zstd Content-Encoding", [zstd_compressed_css] {
        Responses responses;
        responses["hax://example.com"s] = Response{
                .status_line = {.status_code = 200},
                .body{"<html><head><link rel=stylesheet href=lol.css /></head></html>"},
        };
        responses["hax://example.com/lol.css"s] = Response{
                .status_line = {.status_code = 200},
                .headers{{"Content-Encoding", "zstd"}},
                .body{zstd_compressed_css},
        };
        engine::Engine e{std::make_unique<FakeProtocolHandler>(responses)};
        auto page = e.navigate(uri::Uri::parse("hax://example.com").value()).value();
        expect(std::ranges::find(page->stylesheet.rules,
                       css::Rule{
                               .selectors{"p"},
                               .declarations{{css::PropertyId::FontSize, "123em"}},
                       })
                != end(page->stylesheet.rules));
    });

    etest::test("stylesheet link, zstd decoding failure", [zstd_compressed_css]() mutable {
        zstd_compressed_css[0] += 1;
        Responses responses;
        responses["hax://example.com"s] = Response{
                .status_line = {.status_code = 200},
                .body{"<html><head><link rel=stylesheet href=lol.css /></head></html>"},
        };
        responses["hax://example.com/lol.css"s] = Response{
                .status_line = {.status_code = 200},
                .headers{{"Content-Encoding", "zstd"}},
                .body{zstd_compressed_css},
        };
        engine::Engine e{std::make_unique<FakeProtocolHandler>(responses)};
        auto page = e.navigate(uri::Uri::parse("hax://example.com").value()).value();
        expect(std::ranges::find(page->stylesheet.rules,
                       css::Rule{
                               .selectors{"p"},
                               .declarations{{css::PropertyId::FontSize, "123em"}},
                       })
                == end(page->stylesheet.rules));
    });

    etest::test("redirect", [] {
        Responses responses;
        responses["hax://example.com"s] = Response{
                .status_line = {.status_code = 301},
                .headers = {{"Location", "hax://example.com/redirected"}},
        };
        responses["hax://example.com/redirected"s] = Response{
                .status_line = {.status_code = 200},
                .body{"<html><body>hello!</body></html>"},
        };
        engine::Engine e{std::make_unique<FakeProtocolHandler>(std::move(responses))};
        auto page = e.navigate(uri::Uri::parse("hax://example.com").value()).value();
        expect_eq(page->uri.uri, "hax://example.com/redirected");

        auto const &body = std::get<dom::Element>(page->dom.html().children.at(1));
        expect_eq(std::get<dom::Text>(body.children.at(0)).text, "hello!"sv);
    });

    etest::test("redirect not providing Location header", [] {
        Responses responses;
        responses["hax://example.com"s] = Response{
                .status_line = {.status_code = 301},
        };
        engine::Engine e{std::make_unique<FakeProtocolHandler>(std::move(responses))};
        expect_eq(e.navigate(uri::Uri::parse("hax://example.com").value()).error().response.err,
                protocol::ErrorCode::InvalidResponse);
    });

    etest::test("redirect, style", [] {
        Responses responses;
        responses["hax://example.com"s] = Response{
                .status_line = {.status_code = 200},
                .body{"<html><head>"
                      "<link rel=stylesheet href=hello.css />"
                      "</head></html>"},
        };
        responses["hax://example.com/hello.css"s] = Response{
                .status_line = {.status_code = 301},
                .headers = {{"Location", "hax://example.com/redirected.css"}},
        };
        responses["hax://example.com/redirected.css"s] = Response{
                .status_line = {.status_code = 200},
                .body{"p { color: green; }"},
        };
        engine::Engine e{std::make_unique<FakeProtocolHandler>(std::move(responses))};
        auto page = e.navigate(uri::Uri::parse("hax://example.com").value()).value();
        expect(contains(page->stylesheet.rules, {.selectors{"p"}, .declarations{{css::PropertyId::Color, "green"}}}));
    });

    etest::test("redirect loop", [] {
        Responses responses;
        responses["hax://example.com"s] = Response{
                .status_line = {.status_code = 301},
                .headers = {{"Location", "hax://example.com"}},
        };
        engine::Engine e{std::make_unique<FakeProtocolHandler>(std::move(responses))};
        expect_eq(e.navigate(uri::Uri::parse("hax://example.com").value()).error().response.err, //
                protocol::ErrorCode::RedirectLimit);
    });

    etest::test("load", [] {
        Responses responses;
        responses["hax://example.com"s] = Response{
                .status_line = {.status_code = 301},
                .headers = {{"Location", "hax://example.com/redirected"}},
        };
        responses["hax://example.com/redirected"s] = Response{
                .status_line = {.status_code = 200},
                .body{"<html><body>hello!</body></html>"},
        };
        engine::Engine e{std::make_unique<FakeProtocolHandler>(responses)};
        auto res = e.load(uri::Uri::parse("hax://example.com").value());
        expect_eq(res.uri_after_redirects, uri::Uri::parse("hax://example.com/redirected"));
        expect_eq(res.response, responses.at("hax://example.com/redirected"));
    });

    etest::test("IType accessor, you get what you give", [] {
        auto naive = std::make_unique<type::NaiveType>();
        type::IType const *saved = naive.get();
        engine::Engine e{std::make_unique<FakeProtocolHandler>(Responses{}), std::move(naive)};
        expect_eq(&e.font_system(), saved);
    });

    etest::test("bad uri in redirect", [] {
        Responses responses;
        responses["hax://example.com"s] = Response{
                .status_line = {.status_code = 301},
                .headers = {{"Location", ""}},
        };
        engine::Engine e{std::make_unique<FakeProtocolHandler>(responses)};
        auto res = e.navigate(uri::Uri::parse("hax://example.com").value());
        expect_eq(res.error().response.err, protocol::ErrorCode::InvalidResponse);
    });

    etest::test("bad uri in style href", [] {
        Responses responses;
        std::string body = "<html><head><link rel=stylesheet href=";
        body += std::string(1025, 'a');
        body += " /></head></html>";
        responses["hax://example.com"s] = Response{
                .status_line = {.status_code = 200},
                .body{std::move(body)},
        };
        engine::Engine e{std::make_unique<FakeProtocolHandler>(std::move(responses))};
        auto page = e.navigate(uri::Uri::parse("hax://example.com").value());
        expect(page.has_value());
    });

    return etest::run_all_tests();
}
