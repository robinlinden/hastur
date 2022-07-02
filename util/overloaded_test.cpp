// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "util/overloaded.h"

#include "etest/etest.h"

#include <string_view>
#include <variant>

using namespace std::literals;
using etest::expect_eq;
using util::Overloaded;

int main() {
    etest::test("it does what it should", [] {
        auto visitor = Overloaded{
                [](bool) { return "bool"sv; },
                [](int) { return "int"sv; },
                [](long) { return "long"sv; },
        };

        expect_eq("bool"sv, std::visit(visitor, std::variant<bool>(true)));
        expect_eq("int"sv, std::visit(visitor, std::variant<int>(1)));
        expect_eq("long"sv, std::visit(visitor, std::variant<long>(1L)));
    });

    return etest::run_all_tests();
}
