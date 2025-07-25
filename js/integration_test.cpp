// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "js/ast.h"
#include "js/interpreter.h"
#include "js/parser.h"

#include "etest/etest2.h"

#include <format>
#include <optional>

int main() {
    etest::Suite s{};

    s.add_test("foo()", [](etest::IActions &a) {
        js::ast::Interpreter e;
        e.variables["foo"] = js::ast::Value{js::ast::NativeFunction{[](auto const &) {
            return js::ast::Value{42};
        }}};

        auto p = js::Parser::parse("foo()").value();
        a.expect_eq(e.execute(p), js::ast::Value{42});
    });

    s.add_test("foo(1, 2)", [](etest::IActions &a) {
        js::ast::Interpreter e;
        e.variables["foo"] = js::ast::Value{js::ast::NativeFunction{[](auto const &args) {
            return js::ast::Value{args.at(0).as_number() + args.at(1).as_number()};
        }}};

        auto p = js::Parser::parse("foo(1, 2)").value();
        a.expect_eq(e.execute(p), js::ast::Value{3.});
    });

    s.add_test("foo('bar')", [](etest::IActions &a) {
        js::ast::Interpreter e;
        e.variables["foo"] = js::ast::Value{js::ast::NativeFunction{[](auto const &args) {
            return js::ast::Value{args.at(0).as_string()};
        }}};

        auto p = js::Parser::parse("foo('bar')").value();
        a.expect_eq(e.execute(p), js::ast::Value{"bar"});
    });

    s.add_test("foo(1, \"bar\")", [](etest::IActions &a) {
        js::ast::Interpreter e;
        e.variables["foo"] = js::ast::Value{js::ast::NativeFunction{[](auto const &args) {
            return js::ast::Value{
                    std::format("{}: {}", args.at(1).as_string(), args.at(0).as_number()),
            };
        }}};

        auto p = js::Parser::parse("foo(1, \"bar\")").value();
        a.expect_eq(e.execute(p), js::ast::Value{"bar: 1"});
    });

    s.add_test("foo(hello)", [](etest::IActions &a) {
        js::ast::Interpreter e;
        e.variables["foo"] = js::ast::Value{js::ast::NativeFunction{[](auto const &args) {
            return js::ast::Value{args.at(0).as_string()};
        }}};
        e.variables["hello"] = js::ast::Value{"fantastic"};

        auto p = js::Parser::parse("foo(hello)").value();
        a.expect_eq(e.execute(p), js::ast::Value{"fantastic"});
    });

    return s.run();
}
