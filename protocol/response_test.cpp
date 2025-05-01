// SPDX-FileCopyrightText: 2021-2022 Mikael Larsson <c.mikael.larsson@gmail.com>
// SPDX-FileCopyrightText: 2023-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "protocol/response.h"

#include "etest/etest2.h"

#include <cstddef>
#include <string_view>
#include <type_traits>

using namespace std::string_view_literals;

int main() {
    etest::Suite s;

    s.add_test("headers", [](etest::IActions &a) {
        protocol::Headers headers;

        headers.add({"Transfer-Encoding", "chunked"});
        headers.add({"Content-Type", "text/html"});

        a.expect(!headers.get("foo"sv));
        a.expect_eq(headers.get("Transfer-Encoding"sv).value(), "chunked");
        a.expect_eq(headers.get("transfer-encoding"sv).value(), "chunked");
        a.expect_eq(headers.get("CONTENT-TYPE"sv).value(), "text/html");
        a.expect_eq(headers.get("cOnTeNt-TyPe"sv).value(), "text/html");
    });

    s.add_test("headers, init-list", [](etest::IActions &a) {
        protocol::Headers headers{{"Content-Type", "text/html"}};
        a.expect_eq(headers.size(), std::size_t{1});
        a.expect_eq(headers.get("CONTENT-TYPE"sv).value(), "text/html");
        a.expect_eq(headers.get("cOnTeNt-TyPe"sv).value(), "text/html");
    });

    s.add_test("ErrorCode, to_string", [](etest::IActions &a) {
        using protocol::ErrorCode;
        a.expect_eq(to_string(ErrorCode::Unresolved), "Unresolved"sv);
        a.expect_eq(to_string(ErrorCode::Unhandled), "Unhandled"sv);
        a.expect_eq(to_string(ErrorCode::InvalidResponse), "InvalidResponse"sv);
        a.expect_eq(to_string(ErrorCode::RedirectLimit), "RedirectLimit"sv);
        // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
        a.expect_eq(to_string(static_cast<ErrorCode>(std::underlying_type_t<ErrorCode>{20})), "Unknown"sv);
    });

    s.add_test("Headers, to_string()", [](etest::IActions &a) {
        // We don't preserve the order of headers, so let's just test one header.
        protocol::Headers headers{
                {"Set-Cookie", "hello"},
        };

        a.expect_eq(headers.to_string(), "Set-Cookie: hello\n");
    });

    return s.run();
}
