// SPDX-FileCopyrightText: 2022-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "css/property_id.h"

#include "etest/etest2.h"

#include <fmt/core.h>

#include <string_view>

using namespace std::literals;

int main() {
    etest::Suite s;

    s.add_test("property_id_from_string", [](etest::IActions &a) {
        a.expect_eq(css::property_id_from_string("width"), css::PropertyId::Width);
        a.expect_eq(css::property_id_from_string("aaaaa"), css::PropertyId::Unknown);
    });

    s.add_test("to_string", [](etest::IActions &a) {
        a.expect_eq(css::to_string(css::PropertyId::Width), "width");
        a.expect_eq(css::to_string(css::PropertyId::Unknown), "unknown");
    });

    s.add_test("all ids have strings", [](etest::IActions &a) {
        auto id = static_cast<int>(css::PropertyId::Unknown) + 1;
        // Requires a manual update every time we add something last in the enum.
        while (id <= static_cast<int>(css::PropertyId::WordSpacing)) {
            a.expect(css::to_string(static_cast<css::PropertyId>(id)) != "unknown"sv,
                    fmt::format("Property {} is missing a string mapping", id));
            id += 1;
        }
    });

    return s.run();
}
