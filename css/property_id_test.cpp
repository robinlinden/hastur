// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "css/property_id.h"

#include "etest/etest.h"

#include <fmt/core.h>

#include <string_view>

using etest::expect;
using etest::expect_eq;
using namespace std::literals;

int main() {
    etest::test("property_id_from_string", [] {
        expect_eq(css::property_id_from_string("width"), css::PropertyId::Width);
        expect_eq(css::property_id_from_string("aaaaa"), css::PropertyId::Unknown);
    });

    etest::test("to_string", [] {
        expect_eq(css::to_string(css::PropertyId::Width), "width");
        expect_eq(css::to_string(css::PropertyId::Unknown), "unknown");
    });

    etest::test("all ids have strings", [] {
        auto id = static_cast<int>(css::PropertyId::Unknown) + 1;
        // Requires a manual update every time we add something last in the enum.
        while (id <= static_cast<int>(css::PropertyId::WordSpacing)) {
            expect(css::to_string(static_cast<css::PropertyId>(id)) != "unknown"sv,
                    fmt::format("Property {} is missing a string mapping", id));
            id += 1;
        }
    });

    return etest::run_all_tests();
}
