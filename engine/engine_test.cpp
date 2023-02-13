// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "engine/engine.h"

#include "etest/etest.h"
#include "protocol/iprotocol_handler.h"
#include "protocol/response.h"
#include "uri/uri.h"

#include <utility>

using namespace std::literals;
using etest::expect;
using etest::expect_eq;
using etest::require;

namespace {

class FakeProtocolHandler final : public protocol::IProtocolHandler {
public:
    explicit FakeProtocolHandler(protocol::Response response) : response_{std::move(response)} {}
    [[nodiscard]] protocol::Response handle(uri::Uri const &) override { return response_; }

private:
    protocol::Response response_;
};

} // namespace

int main() {
    etest::test("no handlers set", [] {
        engine::Engine e{std::make_unique<FakeProtocolHandler>(protocol::Response{.err = protocol::Error::Unresolved})};
        e.navigate(uri::Uri::parse("hax://example.com"));

        e = engine::Engine{std::make_unique<FakeProtocolHandler>(protocol::Response{.err = protocol::Error::Ok})};
        e.navigate(uri::Uri::parse("hax://example.com"));
        e.set_layout_width(10);
    });

    etest::test("css in <head><style>", [] {
        engine::Engine e{std::make_unique<FakeProtocolHandler>(protocol::Response{
                .err = protocol::Error::Ok,
                .status_line = {.status_code = 200},
                .body{"<html><head><style>p { font-size: 123em; }</style></head></html>"},
        })};
        e.navigate(uri::Uri::parse("hax://example.com"));
        expect_eq(e.stylesheet().back(),
                css::Rule{
                        .selectors{"p"},
                        .declarations{{css::PropertyId::FontSize, "123em"}},
                });
    });

    etest::test("navigation failure", [] {
        bool success{false};
        engine::Engine e{std::make_unique<FakeProtocolHandler>(protocol::Response{.err = protocol::Error::Unresolved})};
        e.set_on_navigation_failure([&](protocol::Error err) { success = err != protocol::Error::Ok; });
        e.set_on_page_loaded([] { require(false); });
        e.set_on_layout_updated([] { require(false); });

        e.navigate(uri::Uri::parse("hax://example.com"));
        expect(success);
    });

    etest::test("page load", [] {
        bool success{false};
        engine::Engine e{std::make_unique<FakeProtocolHandler>(protocol::Response{.err = protocol::Error::Ok})};
        e.set_on_navigation_failure([&](protocol::Error) { require(false); });
        e.set_on_page_loaded([&] { success = true; });
        e.set_on_layout_updated([] { require(false); });

        e.navigate(uri::Uri::parse("hax://example.com"));
        expect(success);
    });

    etest::test("layout update", [] {
        engine::Engine e{std::make_unique<FakeProtocolHandler>(protocol::Response{.err = protocol::Error::Ok})};
        e.set_on_navigation_failure([&](protocol::Error) { require(false); });
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
        engine::Engine e{std::make_unique<FakeProtocolHandler>(protocol::Response{
                .err = protocol::Error::Ok,
                .status_line = {.status_code = 200},
                .body{"<html></html>"},
        })};
        e.navigate(uri::Uri::parse("hax://example.com"));
        // Our default CSS gives <html> the property display: block.
        require(e.layout());
        expect_eq(e.layout()->get_property<css::PropertyId::Display>(), style::DisplayValue::Block);

        e = engine::Engine{std::make_unique<FakeProtocolHandler>(protocol::Response{
                .err = protocol::Error::Ok,
                .status_line = {.status_code = 200},
                .body{"<html><head><style>html { display: inline; }</style></head></html>"},
        })};
        e.navigate(uri::Uri::parse("hax://example.com"));

        // The CSS declared in the page should have a higher priority and give
        // <html> the property display: inline.
        require(e.layout());
        expect_eq(e.layout()->get_property<css::PropertyId::Display>(), style::DisplayValue::Inline);
    });

    return etest::run_all_tests();
}
