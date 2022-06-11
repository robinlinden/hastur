// SPDX-FileCopyrightText: 2021-2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "protocol/http.h"

#include "etest/etest.h"

#include <string_view>

using namespace std::string_view_literals;

using etest::expect;
using etest::expect_eq;

int main() {
    etest::test("headers", [] {
        protocol::Headers headers;

        headers.add({"Transfer-Encoding", "chunked"});
        headers.add({"Content-Type", "text/html"});

        expect(!headers.get("foo"sv));
        expect_eq(headers.get("Transfer-Encoding"sv).value(), "chunked");
        expect_eq(headers.get("transfer-encoding"sv).value(), "chunked");
        expect_eq(headers.get("CONTENT-TYPE"sv).value(), "text/html");
        expect_eq(headers.get("cOnTeNt-TyPe"sv).value(), "text/html");
    });

    return etest::run_all_tests();
}
