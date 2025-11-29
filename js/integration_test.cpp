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
        e.variables["foo"] = js::ast::Value{[](auto const &) {
            return js::ast::Value{42}; //
        }};

        auto p = js::Parser::parse("foo()").value();
        a.expect_eq(e.execute(p), js::ast::Value{42});
    });

    s.add_test("foo();", [](etest::IActions &a) {
        js::ast::Interpreter e;
        e.variables["foo"] = js::ast::Value{[](auto const &) {
            return js::ast::Value{42}; //
        }};

        auto p = js::Parser::parse("foo();").value();
        a.expect_eq(e.execute(p), js::ast::Value{42});
    });

    s.add_test("foo(1, 2)", [](etest::IActions &a) {
        js::ast::Interpreter e;
        e.variables["foo"] = js::ast::Value{[](auto const &args) {
            return js::ast::Value{args.at(0).as_number() + args.at(1).as_number()}; //
        }};

        auto p = js::Parser::parse("foo(1, 2)").value();
        a.expect_eq(e.execute(p), js::ast::Value{3.});
    });

    s.add_test("foo('bar')", [](etest::IActions &a) {
        js::ast::Interpreter e;
        e.variables["foo"] = js::ast::Value{[](auto const &args) {
            return js::ast::Value{args.at(0).as_string()}; //
        }};

        auto p = js::Parser::parse("foo('bar')").value();
        a.expect_eq(e.execute(p), js::ast::Value{"bar"});
    });

    s.add_test("foo(1, \"bar\")", [](etest::IActions &a) {
        js::ast::Interpreter e;
        e.variables["foo"] = js::ast::Value{[](auto const &args) {
            return js::ast::Value{
                    std::format("{}: {}", args.at(1).as_string(), args.at(0).as_number()),
            };
        }};

        auto p = js::Parser::parse("foo(1, \"bar\")").value();
        a.expect_eq(e.execute(p), js::ast::Value{"bar: 1"});
    });

    s.add_test("foo(hello)", [](etest::IActions &a) {
        js::ast::Interpreter e;
        e.variables["foo"] = js::ast::Value{[](auto const &args) {
            return js::ast::Value{args.at(0).as_string()}; //
        }};
        e.variables["hello"] = js::ast::Value{"fantastic"};

        auto p = js::Parser::parse("foo(hello)").value();
        a.expect_eq(e.execute(p), js::ast::Value{"fantastic"});
    });

    s.add_test("foo(1, 2); bar(3, 4)", [](etest::IActions &a) {
        int i = 0; // Hack to check that the functions are called in order.
        js::ast::Interpreter e;
        e.variables["add"] = js::ast::Value{[&](auto const &args) {
            i = 7;
            return js::ast::Value{args.at(0).as_number() + args.at(1).as_number()};
        }};
        e.variables["mul"] = js::ast::Value{[&](auto const &args) {
            i *= 2;
            return js::ast::Value{args.at(0).as_number() * args.at(1).as_number()};
        }};

        auto p = js::Parser::parse("add(1, 2); mul(3, 4)").value();
        a.expect_eq(e.execute(p), js::ast::Value{12.});
        a.expect_eq(i, 14);
    });

    s.add_test("foo(1, 2); bar(3, 4);", [](etest::IActions &a) {
        int i = 0; // Hack to check that the functions are called in order.
        js::ast::Interpreter e;
        e.variables["add"] = js::ast::Value{[&](auto const &args) {
            i = 7;
            return js::ast::Value{args.at(0).as_number() + args.at(1).as_number()};
        }};
        e.variables["mul"] = js::ast::Value{[&](auto const &args) {
            i *= 2;
            return js::ast::Value{args.at(0).as_number() * args.at(1).as_number()};
        }};

        auto p = js::Parser::parse("add(1, 2); mul(3, 4);").value();
        a.expect_eq(e.execute(p), js::ast::Value{12.});
        a.expect_eq(i, 14);
    });

    s.add_test("a = 2; b = 3; c = a; add(a, b, c)", [](etest::IActions &a) {
        js::ast::Interpreter e;
        e.variables["add"] = js::ast::Value{[](auto const &args) {
            return js::ast::Value{args.at(0).as_number() + args.at(1).as_number() + args.at(2).as_number()}; //
        }};

        auto p = js::Parser::parse("a = 2; b = 3; c = a; add(a, b, c)").value();
        a.expect_eq(e.execute(p), js::ast::Value{7.});
    });

    return s.run();
}
