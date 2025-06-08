// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "html2/parse_error.h"

#include "etest/etest2.h"

#include <string>

int main() {
    etest::Suite s;

    s.add_test("to_string(ParseError)", [](etest::IActions &a) {
        // This test will fail if we add new first or last errors, but that's fine.
        constexpr auto kFirstError = html2::ParseError::AbruptClosingOfEmptyComment;
        constexpr auto kLastError = html2::ParseError::UnknownNamedCharacterReference;

        auto error = static_cast<int>(kFirstError);
        a.expect_eq(error, 0);

        while (error <= static_cast<int>(kLastError)) {
            a.expect(to_string(static_cast<html2::ParseError>(error)) != "Unknown error",
                    std::to_string(error) + " is missing an error message");
            error += 1;
        }

        a.expect_eq(to_string(static_cast<html2::ParseError>(error + 1)), "Unknown error");
    });

    return s.run();
}
