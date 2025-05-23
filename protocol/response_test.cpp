// SPDX-FileCopyrightText: 2021-2022 Mikael Larsson <c.mikael.larsson@gmail.com>
// SPDX-FileCopyrightText: 2023-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "protocol/response.h"

#include "etest/etest2.h"

#include <type_traits>

int main() {
    etest::Suite s;

    s.add_test("ErrorCode, to_string", [](etest::IActions &a) {
        using protocol::ErrorCode;
        a.expect_eq(to_string(ErrorCode::Unresolved), "Unresolved");
        a.expect_eq(to_string(ErrorCode::Unhandled), "Unhandled");
        a.expect_eq(to_string(ErrorCode::InvalidResponse), "InvalidResponse");
        a.expect_eq(to_string(ErrorCode::RedirectLimit), "RedirectLimit");
        // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
        a.expect_eq(to_string(static_cast<ErrorCode>(std::underlying_type_t<ErrorCode>{20})), "Unknown");
    });

    s.add_test("to_string(Headers)", [](etest::IActions &a) {
        // The insertion order is preserved for values with the same key.
        protocol::Headers headers{
                {"Set-Cookie", "hello"},
                {"Set-Cookie", "goodbye"},
        };

        a.expect_eq(protocol::to_string(headers), "Set-Cookie: hello\nSet-Cookie: goodbye\n");
    });

    return s.run();
}
