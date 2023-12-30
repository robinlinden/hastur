// SPDX-FileCopyrightText: 2021-2022 Mikael Larsson <c.mikael.larsson@gmail.com>
// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "protocol/response.h"

#include "etest/etest.h"

#include <cstddef>
#include <string_view>
#include <type_traits>

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

    etest::test("headers, init-list", [] {
        protocol::Headers headers{{"Content-Type", "text/html"}};
        expect_eq(headers.size(), std::size_t{1});
        expect_eq(headers.get("CONTENT-TYPE"sv).value(), "text/html");
        expect_eq(headers.get("cOnTeNt-TyPe"sv).value(), "text/html");
    });

    etest::test("error, to_string", [] {
        using protocol::Error;
        expect_eq(to_string(Error::Ok), "Ok"sv);
        expect_eq(to_string(Error::Unresolved), "Unresolved"sv);
        expect_eq(to_string(Error::Unhandled), "Unhandled"sv);
        expect_eq(to_string(Error::InvalidResponse), "InvalidResponse"sv);
        expect_eq(to_string(Error::RedirectLimit), "RedirectLimit"sv);
        expect_eq(to_string(static_cast<Error>(std::underlying_type_t<Error>{20})), "Unknown"sv);
    });

    return etest::run_all_tests();
}
