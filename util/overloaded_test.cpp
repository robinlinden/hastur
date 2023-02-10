// SPDX-FileCopyrightText: 2022-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "util/overloaded.h"

#include "etest/etest.h"

#include <cstdint>
#include <string_view>
#include <variant>

using namespace std::literals;
using etest::expect_eq;
using util::Overloaded;

int main() {
    etest::test("it does what it should", [] {
        auto visitor = Overloaded{
                [](bool) { return "bool"sv; },
                [](std::int32_t) { return "int32_t"sv; },
                [](std::int64_t) { return "int64_t"sv; },
        };

        expect_eq("bool"sv, std::visit(visitor, std::variant<bool>(true)));
        expect_eq("int32_t"sv, std::visit(visitor, std::variant<std::int32_t>(1)));
        expect_eq("int64_t"sv, std::visit(visitor, std::variant<std::int64_t>(1)));
    });

    return etest::run_all_tests();
}
