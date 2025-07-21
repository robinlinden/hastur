// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "js/parser.h"

#include "js/ast.h"

#include "etest/etest2.h"

#include <cstddef>
#include <optional>

int main() {
    etest::Suite s{};

    s.add_test("~", [](etest::IActions &a) {
        auto p = js::Parser::parse("~");
        a.expect_eq(p, std::nullopt);
    });

    s.add_test("()", [](etest::IActions &a) {
        auto p = js::Parser::parse("()");
        a.expect_eq(p, std::nullopt);
    });

    s.add_test("(((", [](etest::IActions &a) {
        auto p = js::Parser::parse("(((");
        a.expect_eq(p, std::nullopt);
    });

    s.add_test("foo()", [](etest::IActions &a) {
        auto p = js::Parser::parse("foo()").value();

        a.expect_eq(p.body.size(), std::size_t{1});
        auto &statement = p.body.at(0);
        auto &expr = std::get<js::ast::ExpressionStatement>(statement).expression;
        auto &call = std::get<js::ast::CallExpression>(expr);
        a.expect_eq(std::get<js::ast::Identifier>(*call.callee).name, "foo");
        a.expect_eq(call.arguments.size(), std::size_t{0});
    });

    s.add_test("foo(", [](etest::IActions &a) {
        auto p = js::Parser::parse("foo(");
        a.expect_eq(p, std::nullopt);
    });

    s.add_test("foo(1,", [](etest::IActions &a) {
        auto p = js::Parser::parse("foo(1,");
        a.expect_eq(p, std::nullopt);
    });

    s.add_test("foo(,2)", [](etest::IActions &a) {
        auto p = js::Parser::parse("foo(,2)");
        a.expect_eq(p, std::nullopt);
    });

    s.add_test("foo(+)", [](etest::IActions &a) {
        auto p = js::Parser::parse("foo(+)");
        a.expect_eq(p, std::nullopt);
    });

    s.add_test("foo(1, 2)", [](etest::IActions &a) {
        auto p = js::Parser::parse("foo(1, 2)").value();

        a.expect_eq(p.body.size(), std::size_t{1});
        auto &statement = p.body.at(0);
        auto &expr = std::get<js::ast::ExpressionStatement>(statement).expression;
        auto &call = std::get<js::ast::CallExpression>(expr);
        a.expect_eq(std::get<js::ast::Identifier>(*call.callee).name, "foo");
        a.expect_eq(call.arguments.size(), std::size_t{2});
        a.expect_eq(std::get<js::ast::NumericLiteral>(std::get<js::ast::Literal>(*call.arguments.at(0))).value, 1.);
        a.expect_eq(std::get<js::ast::NumericLiteral>(std::get<js::ast::Literal>(*call.arguments.at(1))).value, 2.);
    });

    s.add_test("foo(1 2)", [](etest::IActions &a) {
        auto p = js::Parser::parse("foo(1 2)");
        a.expect_eq(p, std::nullopt);
    });

    s.add_test("foo('bar')", [](etest::IActions &a) {
        auto p = js::Parser::parse("foo('bar')").value();

        a.expect_eq(p.body.size(), std::size_t{1});
        auto &statement = p.body.at(0);
        auto &expr = std::get<js::ast::ExpressionStatement>(statement).expression;
        auto &call = std::get<js::ast::CallExpression>(expr);
        a.expect_eq(std::get<js::ast::Identifier>(*call.callee).name, "foo");
        a.expect_eq(call.arguments.size(), std::size_t{1});
        a.expect_eq(std::get<js::ast::StringLiteral>(std::get<js::ast::Literal>(*call.arguments.at(0))).value, "bar");
    });

    s.add_test("foo(1, 'bar')", [](etest::IActions &a) {
        auto p = js::Parser::parse("foo(1, 'bar')").value();

        a.expect_eq(p.body.size(), std::size_t{1});
        auto &statement = p.body.at(0);
        auto &expr = std::get<js::ast::ExpressionStatement>(statement).expression;
        auto &call = std::get<js::ast::CallExpression>(expr);
        a.expect_eq(std::get<js::ast::Identifier>(*call.callee).name, "foo");
        a.expect_eq(call.arguments.size(), std::size_t{2});
        a.expect_eq(std::get<js::ast::NumericLiteral>(std::get<js::ast::Literal>(*call.arguments.at(0))).value, 1.);
        a.expect_eq(std::get<js::ast::StringLiteral>(std::get<js::ast::Literal>(*call.arguments.at(1))).value, "bar");
    });

    s.add_test("foo(hello)", [](etest::IActions &a) {
        auto p = js::Parser::parse("foo(hello)").value();

        a.expect_eq(p.body.size(), std::size_t{1});
        auto &statement = p.body.at(0);
        auto &expr = std::get<js::ast::ExpressionStatement>(statement).expression;
        auto &call = std::get<js::ast::CallExpression>(expr);
        a.expect_eq(std::get<js::ast::Identifier>(*call.callee).name, "foo");
        a.expect_eq(call.arguments.size(), std::size_t{1});
        a.expect_eq(std::get<js::ast::Identifier>(*call.arguments[0]).name, "hello");
    });

    return s.run();
}
