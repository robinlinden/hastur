// SPDX-FileCopyrightText: 2021-2022 Mikael Larsson <c.mikael.larsson@gmail.com>
// SPDX-FileCopyrightText: 2023-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "protocol/response.h"

#include "etest/etest2.h"

#include <string_view>
#include <type_traits>

using namespace std::string_view_literals;

int main() {
    etest::Suite s;

    s.add_test("ErrorCode, to_string", [](etest::IActions &a) {
        using protocol::ErrorCode;
        a.expect_eq(to_string(ErrorCode::Unresolved), "Unresolved"sv);
        a.expect_eq(to_string(ErrorCode::Unhandled), "Unhandled"sv);
        a.expect_eq(to_string(ErrorCode::InvalidResponse), "InvalidResponse"sv);
        a.expect_eq(to_string(ErrorCode::RedirectLimit), "RedirectLimit"sv);
        // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
        a.expect_eq(to_string(static_cast<ErrorCode>(std::underlying_type_t<ErrorCode>{20})), "Unknown"sv);
    });

    s.add_test("to_string(Headers)", [](etest::IActions &a) {
        // We don't preserve the order of headers, so let's just test one header.
        protocol::Headers headers{
                {"Set-Cookie", "hello"},
        };

        a.expect_eq(protocol::to_string(headers), "Set-Cookie: hello\n");
    });

    return s.run();
}
