// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "browser/gui/about_handler.h"

#include "etest/etest2.h"
#include "protocol/response.h"
#include "uri/uri.h"

#include <tl/expected.hpp>

int main() {
    using namespace browser::gui;

    etest::Suite s{};

    s.add_test("Not found", [](etest::IActions &a) {
        AboutHandler handler{{}};

        auto res = handler.handle(uri::Uri::parse("about:nonexistent").value());
        a.expect_eq(res, tl::unexpected{protocol::Error{protocol::ErrorCode::Unresolved}});
    });

    s.add_test("Found", [](etest::IActions &a) {
        AboutHandler handler{Handlers{{
                "blank",
                []() { return "nothing to see here"; },
        }}};

        auto res = handler.handle(uri::Uri::parse("about:blank").value());
        a.expect_eq(res,
                protocol::Response{
                        .status_line = {},
                        .headers = {{"Cache-Control", "no-store"}},
                        .body = "nothing to see here",
                });
    });

    return s.run();
}
