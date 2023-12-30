// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "type/naive.h"

#include "type/type.h"

#include "etest/etest2.h"

int main() {
    etest::Suite s{"type/naive"};

    s.add_test("NaiveFont::measure", [](etest::IActions &a) {
        type::NaiveType type{};

        auto font10px = type.font("a").value();
        a.expect_eq(font10px->measure("a", type::Px{10}), type::Size{5, 10});
        a.expect_eq(font10px->measure("hello", type::Px{10}), type::Size{25, 10});

        auto font20px = type.font("a").value();
        a.expect_eq(font20px->measure("a", type::Px{20}), type::Size{10, 20});
        a.expect_eq(font20px->measure("hello", type::Px{20}), type::Size{50, 20});
    });

    s.add_test("NaiveType::font_cache", [](etest::IActions &a) {
        type::NaiveType type{};

        auto font0 = type.font("a").value();
        auto font1 = type.font("a").value();
        a.expect_eq(font0.get(), font1.get());
    });

    return s.run();
}
