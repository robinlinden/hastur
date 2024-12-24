// SPDX-FileCopyrightText: 2022-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "js/ast.h"

#include "etest/etest2.h"

#include <utility>
#include <vector>

using namespace js::ast;

int main() {
    etest::Suite s{};

    s.add_test("Value: as_bool", [](etest::IActions &a) {
        a.expect_eq(Value{""}.as_bool(), false);
        a.expect_eq(Value{0}.as_bool(), false);
        a.expect_eq(Value{-0}.as_bool(), false);
        a.expect_eq(Value{}.as_bool(), false);

        a.expect_eq(Value{" "}.as_bool(), true);
        a.expect_eq(Value{1}.as_bool(), true);
        a.expect_eq(Value{-0.001}.as_bool(), true);
        a.expect_eq(Value{std::vector<Value>{}}.as_bool(), true);
    });

    s.add_test("Value: object", [](etest::IActions &a) {
        Object o{
                {"hello", Value{5.}}, //
                {"f", Value{NativeFunction{[](std::vector<Value> const &v) {
                     return Value{v.at(0).as_number() * 2};
                 }}}}, //
        };

        auto val = Value{std::move(o)};
        a.require(val.is_object());

        auto const &obj = val.as_object();
        a.expect(obj.contains("hello"));
        a.expect(obj.contains("f"));
        a.expect(!obj.contains("e"));
        a.expect(!obj.contains("henlo"));

        a.expect_eq(obj.at("hello").as_number(), 5.);
        a.expect_eq(obj.at("f").as_native_function().f({Value{5.}}).as_number(), 5. * 2);
    });

    return s.run();
}
