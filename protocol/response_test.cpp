// SPDX-FileCopyrightText: 2021-2022 Mikael Larsson <c.mikael.larsson@gmail.com>
// SPDX-FileCopyrightText: 2023-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "protocol/response.h"

#include "etest/etest2.h"

#include <string>

int main() {
    etest::Suite s;

    s.add_test("ErrorCode, to_string", [](etest::IActions &a) {
        static constexpr auto kFirstError = protocol::ErrorCode::Unresolved;
        static constexpr auto kLastError = protocol::ErrorCode::RedirectLimit;

        auto error = static_cast<int>(kFirstError);
        a.expect_eq(error, 0);

        while (error <= static_cast<int>(kLastError)) {
            a.expect(to_string(static_cast<protocol::ErrorCode>(error)) != "Unknown",
                    std::to_string(error) + " is missing an error message");
            error += 1;
        }

        a.expect_eq(to_string(static_cast<protocol::ErrorCode>(error + 1)), "Unknown");
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
