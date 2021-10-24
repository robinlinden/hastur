// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "browser/engine.h"

#include "etest/etest.h"
#include "protocol/get.h"
#include "uri/uri.h"

using namespace std::literals;
using etest::expect;
using etest::require;

int main() {
    etest::test("navigation failure", [] {
        bool success{false};
        browser::Engine e;
        e.set_on_navigation_failure([&](protocol::Error err) { success = err != protocol::Error::Ok; });
        e.set_on_page_loaded([] { require(false); });
        e.set_on_layout_updated([] { require(false); });

        e.navigate(*uri::Uri::parse("http://aaaaaaaaaaaaaaaa.aaaaaaaaaaaaaaaaaaaa"));
        expect(success);
    });

    etest::test("page load", [] {
        bool success{false};
        browser::Engine e;
        e.set_on_navigation_failure([&](protocol::Error) { require(false); });
        e.set_on_page_loaded([&] { success = true; });
        e.set_on_layout_updated([] { require(false); });

        e.navigate(*uri::Uri::parse("http://example.com"));
        expect(success);
    });

    etest::test("layout update", [] {
        browser::Engine e;
        e.set_on_navigation_failure([&](protocol::Error) { require(false); });
        e.set_on_page_loaded([] { require(false); });
        e.set_on_layout_updated([] { require(false); });

        e.set_layout_width(10);

        bool success{false};
        e.set_on_page_loaded([&] { success = true; });

        e.navigate(*uri::Uri::parse("http://example.com"));

        expect(success);

        e.set_on_page_loaded([&] { require(false); });
        success = false;
        e.set_on_layout_updated([&] { success = true; });

        e.set_layout_width(100);
        expect(success);
    });

    return etest::run_all_tests();
}
