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

    // Same as above, but with a semicolon. We can't just compare the asts due
    // to it containing shared_ptrs.
    s.add_test("foo();", [](etest::IActions &a) {
        auto p = js::Parser::parse("foo();").value();

        a.expect_eq(p.body.size(), std::size_t{1});
        auto &statement = p.body.at(0);
        auto &expr = std::get<js::ast::ExpressionStatement>(statement).expression;
        auto &call = std::get<js::ast::CallExpression>(expr);
        a.expect_eq(std::get<js::ast::Identifier>(*call.callee).name, "foo");
        a.expect_eq(call.arguments.size(), std::size_t{0});
    });

    s.add_test("foo(); bar()", [](etest::IActions &a) {
        auto p = js::Parser::parse("foo(); bar()").value();

        a.expect_eq(p.body.size(), std::size_t{2});
        auto &first_statement = p.body.at(0);
        auto &first_expr = std::get<js::ast::ExpressionStatement>(first_statement).expression;
        auto &first_call = std::get<js::ast::CallExpression>(first_expr);
        a.expect_eq(std::get<js::ast::Identifier>(*first_call.callee).name, "foo");
        a.expect_eq(first_call.arguments.size(), std::size_t{0});

        auto &second_statement = p.body.at(1);
        auto &second_expr = std::get<js::ast::ExpressionStatement>(second_statement).expression;
        auto &second_call = std::get<js::ast::CallExpression>(second_expr);
        a.expect_eq(std::get<js::ast::Identifier>(*second_call.callee).name, "bar");
        a.expect_eq(second_call.arguments.size(), std::size_t{0});
    });

    s.add_test("foo() bar()", [](etest::IActions &a) {
        auto p = js::Parser::parse("foo() bar()");
        a.expect_eq(p, std::nullopt);
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
        a.expect_eq(std::get<js::ast::NumericLiteral>(std::get<js::ast::Literal>(call.arguments.at(0))).value, 1.);
        a.expect_eq(std::get<js::ast::NumericLiteral>(std::get<js::ast::Literal>(call.arguments.at(1))).value, 2.);
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
        a.expect_eq(std::get<js::ast::StringLiteral>(std::get<js::ast::Literal>(call.arguments.at(0))).value, "bar");
    });

    s.add_test("foo(1, 'bar')", [](etest::IActions &a) {
        auto p = js::Parser::parse("foo(1, 'bar')").value();

        a.expect_eq(p.body.size(), std::size_t{1});
        auto &statement = p.body.at(0);
        auto &expr = std::get<js::ast::ExpressionStatement>(statement).expression;
        auto &call = std::get<js::ast::CallExpression>(expr);
        a.expect_eq(std::get<js::ast::Identifier>(*call.callee).name, "foo");
        a.expect_eq(call.arguments.size(), std::size_t{2});
        a.expect_eq(std::get<js::ast::NumericLiteral>(std::get<js::ast::Literal>(call.arguments.at(0))).value, 1.);
        a.expect_eq(std::get<js::ast::StringLiteral>(std::get<js::ast::Literal>(call.arguments.at(1))).value, "bar");
    });

    s.add_test("foo(hello)", [](etest::IActions &a) {
        auto p = js::Parser::parse("foo(hello)").value();

        a.expect_eq(p.body.size(), std::size_t{1});
        auto &statement = p.body.at(0);
        auto &expr = std::get<js::ast::ExpressionStatement>(statement).expression;
        auto &call = std::get<js::ast::CallExpression>(expr);
        a.expect_eq(std::get<js::ast::Identifier>(*call.callee).name, "foo");
        a.expect_eq(call.arguments.size(), std::size_t{1});
        a.expect_eq(std::get<js::ast::Identifier>(call.arguments[0]).name, "hello");
    });

    s.add_test("you(fool", [](etest::IActions &a) {
        a.expect_eq(js::Parser::parse("you(fool"), std::nullopt); //
    });

    s.add_test("assign expr, number", [](etest::IActions &a) {
        auto p = js::Parser::parse("x = 42;").value();

        a.expect_eq(p.body.size(), std::size_t{1});
        auto &statement = p.body.at(0);
        auto &expr = std::get<js::ast::ExpressionStatement>(statement).expression;
        auto &assign = std::get<js::ast::AssignmentExpression>(expr);
        a.expect_eq(std::get<js::ast::Identifier>(*assign.left).name, "x");
        a.expect_eq(std::get<js::ast::NumericLiteral>(std::get<js::ast::Literal>(*assign.right)).value, 42.);
    });

    s.add_test("assign expr, string", [](etest::IActions &a) {
        auto p = js::Parser::parse("y = 'hello';").value();

        a.expect_eq(p.body.size(), std::size_t{1});
        auto &statement = p.body.at(0);
        auto &expr = std::get<js::ast::ExpressionStatement>(statement).expression;
        auto &assign = std::get<js::ast::AssignmentExpression>(expr);
        a.expect_eq(std::get<js::ast::Identifier>(*assign.left).name, "y");
        a.expect_eq(std::get<js::ast::StringLiteral>(std::get<js::ast::Literal>(*assign.right)).value, "hello");
    });

    s.add_test("assign expr, identifier", [](etest::IActions &a) {
        auto p = js::Parser::parse("z = foo;").value();

        a.expect_eq(p.body.size(), std::size_t{1});
        auto &statement = p.body.at(0);
        auto &expr = std::get<js::ast::ExpressionStatement>(statement).expression;
        auto &assign = std::get<js::ast::AssignmentExpression>(expr);
        a.expect_eq(std::get<js::ast::Identifier>(*assign.left).name, "z");
        a.expect_eq(std::get<js::ast::Identifier>(*assign.right).name, "foo");
    });

    s.add_test("assign expr, rhs parse error", [](etest::IActions &a) {
        auto p = js::Parser::parse("x = =");
        a.expect_eq(p, std::nullopt);
    });

    s.add_test("assign expr, function call", [](etest::IActions &a) {
        auto p = js::Parser::parse("a = func(1, 2);").value();

        a.expect_eq(p.body.size(), std::size_t{1});
        auto &statement = p.body.at(0);
        auto &expr = std::get<js::ast::ExpressionStatement>(statement).expression;
        auto &assign = std::get<js::ast::AssignmentExpression>(expr);
        a.expect_eq(std::get<js::ast::Identifier>(*assign.left).name, "a");

        auto &call = std::get<js::ast::CallExpression>(*assign.right);
        a.expect_eq(std::get<js::ast::Identifier>(*call.callee).name, "func");
        a.expect_eq(call.arguments.size(), std::size_t{2});
        a.expect_eq(std::get<js::ast::NumericLiteral>(std::get<js::ast::Literal>(call.arguments.at(0))).value, 1.);
        a.expect_eq(std::get<js::ast::NumericLiteral>(std::get<js::ast::Literal>(call.arguments.at(1))).value, 2.);
    });

    s.add_test("assign expr, chained assignment", [](etest::IActions &a) {
        auto p = js::Parser::parse("x = y = 5;").value();

        a.expect_eq(p.body.size(), std::size_t{1});
        auto &statement = p.body.at(0);
        auto &expr = std::get<js::ast::ExpressionStatement>(statement).expression;
        auto &first_assign = std::get<js::ast::AssignmentExpression>(expr);
        a.expect_eq(std::get<js::ast::Identifier>(*first_assign.left).name, "x");

        auto &second_assign = std::get<js::ast::AssignmentExpression>(*first_assign.right);
        a.expect_eq(std::get<js::ast::Identifier>(*second_assign.left).name, "y");
        a.expect_eq(std::get<js::ast::NumericLiteral>(std::get<js::ast::Literal>(*second_assign.right)).value, 5.);
    });

    s.add_test("member expr", [](etest::IActions &a) {
        auto p = js::Parser::parse("obj.prop;").value();

        a.expect_eq(p.body.size(), std::size_t{1});
        auto &statement = p.body.at(0);
        auto &expr = std::get<js::ast::ExpressionStatement>(statement).expression;
        auto &member = std::get<js::ast::MemberExpression>(expr);
        a.expect_eq(std::get<js::ast::Identifier>(*member.object).name, "obj");
        a.expect_eq(member.property.name, "prop");
    });

    s.add_test("member expr chaining", [](etest::IActions &a) {
        auto p = js::Parser::parse("obj.foo.bar;").value();

        a.expect_eq(p.body.size(), std::size_t{1});
        auto &statement = p.body.at(0);
        auto &expr = std::get<js::ast::ExpressionStatement>(statement).expression;
        auto &first_member = std::get<js::ast::MemberExpression>(expr);
        auto &second_member = std::get<js::ast::MemberExpression>(*first_member.object);
        a.expect_eq(std::get<js::ast::Identifier>(*second_member.object).name, "obj");
        a.expect_eq(second_member.property.name, "foo");
        a.expect_eq(first_member.property.name, "bar");
    });

    s.add_test("member expr assign", [](etest::IActions &a) {
        auto p = js::Parser::parse("obj.prop = 5;").value();

        a.expect_eq(p.body.size(), std::size_t{1});
        auto &statement = p.body.at(0);
        auto &expr = std::get<js::ast::ExpressionStatement>(statement).expression;
        auto &assign = std::get<js::ast::AssignmentExpression>(expr);
        auto &member = std::get<js::ast::MemberExpression>(*assign.left);
        auto &value = std::get<js::ast::NumericLiteral>(std::get<js::ast::Literal>(*assign.right));
        a.expect_eq(value.value, 5.);
        a.expect_eq(std::get<js::ast::Identifier>(*member.object).name, "obj");
        a.expect_eq(member.property.name, "prop");
    });

    s.add_test("assign member expr", [](etest::IActions &a) {
        auto p = js::Parser::parse("obj = other.prop;").value();

        a.expect_eq(p.body.size(), std::size_t{1});
        auto &statement = p.body.at(0);
        auto &expr = std::get<js::ast::ExpressionStatement>(statement).expression;
        auto &assign = std::get<js::ast::AssignmentExpression>(expr);
        a.expect_eq(std::get<js::ast::Identifier>(*assign.left).name, "obj");
        auto &member = std::get<js::ast::MemberExpression>(*assign.right);
        a.expect_eq(std::get<js::ast::Identifier>(*member.object).name, "other");
        a.expect_eq(member.property.name, "prop");
    });

    s.add_test("call member expr", [](etest::IActions &a) {
        auto p = js::Parser::parse("obj.method();").value();

        a.expect_eq(p.body.size(), std::size_t{1});
        auto &statement = p.body.at(0);
        auto &expr = std::get<js::ast::ExpressionStatement>(statement).expression;
        auto &call = std::get<js::ast::CallExpression>(expr);
        auto &member = std::get<js::ast::MemberExpression>(*call.callee);
        a.expect_eq(std::get<js::ast::Identifier>(*member.object).name, "obj");
        a.expect_eq(member.property.name, "method");
        a.expect_eq(call.arguments.size(), std::size_t{0});
    });

    s.add_test("call assign member expr", [](etest::IActions &a) {
        auto p = js::Parser::parse("obj.method = func();").value();

        a.expect_eq(p.body.size(), std::size_t{1});
        auto &statement = p.body.at(0);
        auto &expr = std::get<js::ast::ExpressionStatement>(statement).expression;
        auto &assign = std::get<js::ast::AssignmentExpression>(expr);
        auto &member = std::get<js::ast::MemberExpression>(*assign.left);
        a.expect_eq(std::get<js::ast::Identifier>(*member.object).name, "obj");
        a.expect_eq(member.property.name, "method");

        auto &call = std::get<js::ast::CallExpression>(*assign.right);
        a.expect_eq(std::get<js::ast::Identifier>(*call.callee).name, "func");
        a.expect_eq(call.arguments.size(), std::size_t{0});
    });

    s.add_test("function declaration, bad", [](etest::IActions &a) {
        a.expect_eq(js::Parser::parse("function").has_value(), false);
        a.expect_eq(js::Parser::parse("function 37").has_value(), false);
        a.expect_eq(js::Parser::parse("function foo").has_value(), false);
        a.expect_eq(js::Parser::parse("function foo!").has_value(), false);
        a.expect_eq(js::Parser::parse("function foo(").has_value(), false);
        a.expect_eq(js::Parser::parse("function foo(!").has_value(), false);
        a.expect_eq(js::Parser::parse("function foo()").has_value(), false);
        a.expect_eq(js::Parser::parse("function foo() !").has_value(), false);
        a.expect_eq(js::Parser::parse("function foo() {").has_value(), false);
        a.expect_eq(js::Parser::parse("function foo() {!").has_value(), false);
        a.expect_eq(js::Parser::parse("function foo() { function }").has_value(), false);
        a.expect_eq(js::Parser::parse("function foo() { 42").has_value(), false);
        a.expect_eq(js::Parser::parse("function foo() { 42;").has_value(), false);
        a.expect_eq(js::Parser::parse("function foo() { a b }").has_value(), false);
        a.expect_eq(js::Parser::parse("function foo(~) {}").has_value(), false);
        a.expect_eq(js::Parser::parse("function foo(a b) {}").has_value(), false);
        a.expect_eq(js::Parser::parse("function foo(a").has_value(), false);
        a.expect_eq(js::Parser::parse("function foo(a,").has_value(), false);
        a.expect_eq(js::Parser::parse("function foo(a, 42").has_value(), false);
    });

    s.add_test("function declaration, empty", [](etest::IActions &a) {
        auto p = js::Parser::parse("function foo() {}").value();

        a.expect_eq(p.body.size(), std::size_t{1});
        auto &statement = p.body.at(0);
        auto &decl = std::get<js::ast::Declaration>(statement);
        auto &func_decl = std::get<js::ast::FunctionDeclaration>(decl);
        a.expect_eq(func_decl.id.name, "foo");
        a.expect_eq(func_decl.function->params.size(), std::size_t{0});
        a.expect_eq(func_decl.function->body.body.size(), std::size_t{0});
    });

    s.add_test("function declaration, trailing comma in params", [](etest::IActions &a) {
        auto p = js::Parser::parse("function foo(a, b,) {}").value();

        a.expect_eq(p.body.size(), std::size_t{1});
        auto &statement = p.body.at(0);
        auto &decl = std::get<js::ast::Declaration>(statement);
        auto &func_decl = std::get<js::ast::FunctionDeclaration>(decl);
        a.expect_eq(func_decl.id.name, "foo");
        a.expect_eq(func_decl.function->params.size(), std::size_t{2});
        a.expect_eq(std::get<js::ast::Identifier>(func_decl.function->params.at(0)).name, "a");
        a.expect_eq(std::get<js::ast::Identifier>(func_decl.function->params.at(1)).name, "b");
        a.expect_eq(func_decl.function->body.body.size(), std::size_t{0});
    });

    s.add_test("function declaration, with params and body", [](etest::IActions &a) {
        auto p = js::Parser::parse("function set(a, b) { a = b; }").value();

        a.expect_eq(p.body.size(), std::size_t{1});
        auto &statement = p.body.at(0);
        auto &decl = std::get<js::ast::Declaration>(statement);
        auto &func_decl = std::get<js::ast::FunctionDeclaration>(decl);
        a.expect_eq(func_decl.id.name, "set");
        a.expect_eq(func_decl.function->params.size(), std::size_t{2});
        a.expect_eq(std::get<js::ast::Identifier>(func_decl.function->params.at(0)).name, "a");
        a.expect_eq(std::get<js::ast::Identifier>(func_decl.function->params.at(1)).name, "b");
        a.expect_eq(func_decl.function->body.body.size(), std::size_t{1});
        auto &body_statement = func_decl.function->body.body.at(0);
        auto &body_expr = std::get<js::ast::ExpressionStatement>(body_statement).expression;
        auto &assign = std::get<js::ast::AssignmentExpression>(body_expr);
        a.expect_eq(std::get<js::ast::Identifier>(*assign.left).name, "a");
        a.expect_eq(std::get<js::ast::Identifier>(*assign.right).name, "b");
    });

    s.add_test("return statement", [](etest::IActions &a) {
        auto p = js::Parser::parse("return 42;").value();

        a.expect_eq(p.body.size(), std::size_t{1});
        auto &statement = p.body.at(0);
        auto &ret = std::get<js::ast::ReturnStatement>(statement);
        a.expect_eq(std::get<js::ast::NumericLiteral>(std::get<js::ast::Literal>(*ret.argument)).value, 42.);

        a.expect_eq(js::Parser::parse("return"), std::nullopt);
        a.expect_eq(js::Parser::parse("return )"), std::nullopt);
    });

    s.add_test("return statement, void", [](etest::IActions &a) {
        auto p = js::Parser::parse("return;").value();
        a.expect_eq(p.body.size(), std::size_t{1});
        auto &statement = p.body.at(0);
        auto &ret = std::get<js::ast::ReturnStatement>(statement);
        a.expect_eq(ret.argument.has_value(), false);
    });

    s.add_test("return statement, bad", [](etest::IActions &a) {
        auto p = js::Parser::parse("return ~");
        a.expect_eq(p, std::nullopt);
    });

    return s.run();
}
