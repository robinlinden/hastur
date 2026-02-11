// SPDX-FileCopyrightText: 2025-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "js/ast.h"
#include "js/interpreter.h"
#include "js/parser.h"

#include "etest/etest2.h"

#include <cstddef>
#include <format>
#include <optional>

int main() {
    etest::Suite s{};

    s.add_test("foo();", [](etest::IActions &a) {
        js::ast::Interpreter e;
        e.variables["foo"] = js::ast::Value{[](auto const &) {
            return js::ast::Value{42}; //
        }};

        auto p = js::Parser::parse("foo();").value();
        a.expect_eq(e.execute(p), js::ast::Value{42});
    });

    s.add_test("foo(1, 2);", [](etest::IActions &a) {
        js::ast::Interpreter e;
        e.variables["foo"] = js::ast::Value{[](auto const &args) {
            return js::ast::Value{args.at(0).as_number() + args.at(1).as_number()}; //
        }};

        auto p = js::Parser::parse("foo(1, 2);").value();
        a.expect_eq(e.execute(p), js::ast::Value{3.});
    });

    s.add_test("foo('bar');", [](etest::IActions &a) {
        js::ast::Interpreter e;
        e.variables["foo"] = js::ast::Value{[](auto const &args) {
            return js::ast::Value{args.at(0).as_string()}; //
        }};

        auto p = js::Parser::parse("foo('bar');").value();
        a.expect_eq(e.execute(p), js::ast::Value{"bar"});
    });

    s.add_test("foo(1, \"bar\");", [](etest::IActions &a) {
        js::ast::Interpreter e;
        e.variables["foo"] = js::ast::Value{[](auto const &args) {
            return js::ast::Value{
                    std::format("{}: {}", args.at(1).as_string(), args.at(0).as_number()),
            };
        }};

        auto p = js::Parser::parse("foo(1, \"bar\");").value();
        a.expect_eq(e.execute(p), js::ast::Value{"bar: 1"});
    });

    s.add_test("foo(hello);", [](etest::IActions &a) {
        js::ast::Interpreter e;
        e.variables["foo"] = js::ast::Value{[](auto const &args) {
            return js::ast::Value{args.at(0).as_string()}; //
        }};
        e.variables["hello"] = js::ast::Value{"fantastic"};

        auto p = js::Parser::parse("foo(hello);").value();
        a.expect_eq(e.execute(p), js::ast::Value{"fantastic"});
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

    s.add_test("a = 2; b = 3; c = a; add(a, b, c);", [](etest::IActions &a) {
        js::ast::Interpreter e;
        e.variables["add"] = js::ast::Value{[](auto const &args) {
            return js::ast::Value{args.at(0).as_number() + args.at(1).as_number() + args.at(2).as_number()}; //
        }};

        auto p = js::Parser::parse("a = 2; b = 3; c = a; add(a, b, c);").value();
        a.expect_eq(e.execute(p), js::ast::Value{7.});
    });

    s.add_test("expressions as fn args", [](etest::IActions &a) {
        js::ast::Interpreter e;
        e.variables["add"] = js::ast::Value{[](auto const &args) {
            return js::ast::Value{args.at(0).as_number() + args.at(1).as_number()}; //
        }};

        auto p = js::Parser::parse("add(lol = 2, add(5, 10));").value();
        a.expect_eq(e.execute(p), js::ast::Value{17.});
        a.expect_eq(e.variables.at("lol").as_number(), 2.);
    });

    s.add_test("member expr", [](etest::IActions &a) {
        js::ast::Interpreter e;
        e.variables["obj"] = js::ast::Value{js::ast::Object{
                {"prop", js::ast::Value{123}},
        }};

        auto p1 = js::Parser::parse("obj.prop;").value();
        a.expect_eq(e.execute(p1), js::ast::Value{123});

        auto p2 = js::Parser::parse("a = obj.prop;").value();
        a.expect_eq(e.execute(p2), js::ast::Value{123});
        a.expect_eq(e.variables.at("a").as_number(), 123.);
    });

    s.add_test("function declaration and call, bonus garbage after return", [](etest::IActions &a) {
        auto p = js::Parser::parse("function get_3() { return 3; foo(); }; get_3();").value();
        js::ast::Interpreter e;
        a.expect_eq(e.execute(p), js::ast::Value{3.});
    });

    s.add_test("function declaration and call, no semicolon after", [](etest::IActions &a) {
        auto p = js::Parser::parse("function get_3() { return 3; } get_3();").value();
        js::ast::Interpreter e;
        a.expect_eq(e.execute(p), js::ast::Value{3.});
    });

    s.add_test("function declaration and call, void return", [](etest::IActions &a) {
        auto p = js::Parser::parse("function get_nothing() { return; foo(); }; get_nothing();").value();
        js::ast::Interpreter e;
        a.expect_eq(e.execute(p), js::ast::Value{});
    });

    s.add_test("function declaration and call, with args", [](etest::IActions &a) {
        auto p = js::Parser::parse("function add(a, b, c) { return native_add(a, b, c); }; add(37, 3, 2);").value();
        js::ast::Interpreter e;
        e.variables["native_add"] = js::ast::Value{[&](auto const &args) {
            a.expect_eq(args.size(), std::size_t{3});
            a.expect_eq(args.at(0).as_number(), 37.);
            a.expect_eq(args.at(1).as_number(), 3.);
            a.expect_eq(args.at(2).as_number(), 2.);
            return js::ast::Value{args.at(0).as_number() + args.at(1).as_number() + args.at(2).as_number()};
        }};
        a.expect_eq(e.execute(p), js::ast::Value{42.});
    });

    s.add_test("string literal member expr", [](etest::IActions &a) {
        auto p = js::Parser::parse("'foo'.length;").value();
        // TODO(robinlinden): a.expect_eq(js::ast::Interpreter{}.execute(p), js::ast::Value{3});
        a.expect_eq(js::ast::Interpreter{}.execute(p).has_value(), false);
    });

    s.add_test("function expression and call", [](etest::IActions &a) {
        auto p = js::Parser::parse("a = function(a, b) { return b; }; a(40, 2);").value();
        js::ast::Interpreter e;
        a.expect_eq(e.execute(p), js::ast::Value{2.});
    });

    return s.run();
}
