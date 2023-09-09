// SPDX-FileCopyrightText: 2022-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "js/ast.h"

#include "etest/etest.h"

using namespace js::ast;
using etest::expect_eq;

int main() {
    etest::test("Value: as_bool", [] {
        expect_eq(Value{""}.as_bool(), false);
        expect_eq(Value{0}.as_bool(), false);
        expect_eq(Value{-0}.as_bool(), false);
        expect_eq(Value{}.as_bool(), false);

        expect_eq(Value{" "}.as_bool(), true);
        expect_eq(Value{1}.as_bool(), true);
        expect_eq(Value{-0.001}.as_bool(), true);
        expect_eq(Value{std::vector<Value>{}}.as_bool(), true);
    });

    etest::test("Value: object", [] {
        Object o{
                {"hello", Value{5.}}, //
                {"f", Value{NativeFunction{[](std::vector<Value> const &v) {
                     return Value{v.at(0).as_number() * 2};
                 }}}}, //
        };

        auto val = Value{std::move(o)};
        etest::require(val.is_object());

        auto const &obj = val.as_object();
        etest::expect(obj.contains("hello"));
        etest::expect(obj.contains("f"));
        etest::expect(!obj.contains("e"));
        etest::expect(!obj.contains("henlo"));

        expect_eq(obj.at("hello").as_number(), 5.);
        expect_eq(obj.at("f").as_native_function().f({Value{5.}}).as_number(), 5. * 2);
    });

    return etest::run_all_tests();
}
