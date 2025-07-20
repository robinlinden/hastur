// SPDX-FileCopyrightText: 2022-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "js/interpreter.h"

#include "js/ast.h"

#include "etest/etest2.h"

#include <cstddef>
#include <memory>
#include <string>
#include <tuple>
#include <utility>

using namespace js::ast;

int main() {
    etest::Suite s{};

    s.add_test("literals", [](etest::IActions &a) {
        Interpreter e;
        a.expect_eq(e.execute(NumericLiteral{5.}), Value{5.});
        a.expect_eq(e.execute(StringLiteral{"hello"}), Value{"hello"});
    });

    s.add_test("binary expression, plus", [](etest::IActions &a) {
        auto plus_expr = BinaryExpression{
                .op = BinaryOperator::Plus,
                .lhs = std::make_shared<Expression>(NumericLiteral{11.}),
                .rhs = std::make_shared<Expression>(NumericLiteral{31.}),
        };

        Interpreter e;
        a.expect_eq(e.execute(plus_expr), Value{42.});
    });

    s.add_test("binary expression, identifiers", [](etest::IActions &a) {
        auto plus_expr = BinaryExpression{
                .op = BinaryOperator::Plus,
                .lhs = std::make_shared<Expression>(Identifier{"eleven"}),
                .rhs = std::make_shared<Expression>(Identifier{"thirtyone"}),
        };

        Interpreter e;
        e.variables["eleven"] = Value{11.};
        e.variables["thirtyone"] = Value{31.};
        a.expect_eq(e.execute(plus_expr), Value{42.});
    });

    s.add_test("binary expression, minus", [](etest::IActions &a) {
        auto minus_expr = BinaryExpression{
                .op = BinaryOperator::Minus,
                .lhs = std::make_shared<Expression>(NumericLiteral{11.}),
                .rhs = std::make_shared<Expression>(NumericLiteral{31.}),
        };

        Interpreter e;
        a.expect_eq(e.execute(minus_expr), Value{-20.});
    });

    s.add_test("the ast is copyable", [](etest::IActions &) {
        Program p1;
        auto p2 = p1; // NOLINT(performance-unnecessary-copy-initialization)
        std::ignore = p2;
    });

    s.add_test("variable declaration", [](etest::IActions &a) {
        auto declaration = VariableDeclaration{{
                VariableDeclarator{
                        .id = Identifier{"a"},
                        .init = NumericLiteral{1.},
                },
        }};

        Interpreter e;
        a.expect_eq(e.execute(declaration), Value{});
        a.expect_eq(e.variables, decltype(e.variables){{"a", Value{1.}}});
    });

    s.add_test("function call, arguments", [](etest::IActions &a) {
        auto function_body = ReturnStatement{BinaryExpression{
                .op = BinaryOperator::Plus,
                .lhs = std::make_shared<Expression>(Identifier{"one"}),
                .rhs = std::make_shared<Expression>(Identifier{"two"}),
        }};

        auto declaration = FunctionDeclaration{
                .id = Identifier{"func"},
                .function = std::make_shared<Function>(Function{
                        .params{Identifier{"one"}, Identifier{"two"}},
                        .body{{std::move(function_body)}},
                }),
        };

        auto call = CallExpression{
                .callee = std::make_shared<Expression>(Identifier{"func"}),
                .arguments{
                        std::make_shared<Expression>(NumericLiteral{13.}),
                        std::make_shared<Expression>(NumericLiteral{4.}),
                },
        };

        Interpreter e;
        a.expect_eq(e.execute(declaration), Value{});
        a.expect_eq(e.execute(call), Value{13. + 4.});

        // The only variable in scope should be the function we declared.
        a.expect_eq(e.variables.size(), std::size_t{1});
    });

    s.add_test("member expression", [](etest::IActions &a) {
        Interpreter e;
        e.variables["obj"] = Value{Object{{"hello", Value{5.}}}};

        auto member_expr = MemberExpression{
                .object = std::make_shared<Expression>(Identifier{"obj"}),
                .property = Identifier{"hello"},
        };

        a.expect_eq(e.execute(member_expr), Value{5.});
    });

    s.add_test("return, values are returned", [](etest::IActions &a) {
        auto declaration = FunctionDeclaration{
                .id = Identifier{"func"},
                .function = std::make_shared<Function>(Function{
                        .params{},
                        .body{{ReturnStatement{NumericLiteral{42.}}}},
                }),
        };

        auto call = CallExpression{.callee = std::make_shared<Expression>(Identifier{"func"})};

        Interpreter e;
        a.expect_eq(e.execute(declaration), Value{});
        a.expect_eq(e.execute(call), Value{42.});
    });

    s.add_test("return, function execution is ended", [](etest::IActions &a) {
        auto declaration = FunctionDeclaration{
                .id = Identifier{"func"},
                .function = std::make_shared<Function>(Function{
                        .params{},
                        .body{{
                                ReturnStatement{},
                                ReturnStatement{NumericLiteral{42.}},
                        }},
                }),
        };

        auto call = CallExpression{.callee = std::make_shared<Expression>(Identifier{"func"})};

        Interpreter e;
        a.expect_eq(e.execute(declaration), Value{});
        a.expect_eq(e.execute(call), Value{});
    });

    s.add_test("return, function execution is ended even in while", [](etest::IActions &a) {
        auto declaration = FunctionDeclaration{
                .id = Identifier{"func"},
                .function = std::make_shared<Function>(Function{
                        .params{},
                        .body{{
                                WhileStatement{
                                        .test = NumericLiteral{1},
                                        .body = std::make_shared<Statement>(ReturnStatement{NumericLiteral{37.}}),
                                },
                                ReturnStatement{NumericLiteral{42.}},
                        }},
                }),
        };

        auto call = CallExpression{.callee = std::make_shared<Expression>(Identifier{"func"})};

        Interpreter e;
        a.expect_eq(e.execute(declaration), Value{});
        a.expect_eq(e.execute(call), Value{37.});
    });

    s.add_test("expression statement", [](etest::IActions &a) {
        Interpreter e;
        a.expect_eq(e.execute(ExpressionStatement{StringLiteral{"hi"}}), Value{"hi"});
        a.expect_eq(e.execute(ExpressionStatement{NumericLiteral{1213}}), Value{1213});
    });

    s.add_test("if", [](etest::IActions &a) {
        auto if_stmt = IfStatement{
                .test = NumericLiteral{1},
                .if_branch = std::make_shared<Statement>(ExpressionStatement{StringLiteral{"true!"}}),
        };

        Interpreter e;
        a.expect_eq(e.execute(if_stmt), Value{"true!"});

        if_stmt.test = NumericLiteral{0};
        a.expect_eq(e.execute(if_stmt), Value{});
    });

    s.add_test("if-else", [](etest::IActions &a) {
        auto if_stmt = IfStatement{
                .test = NumericLiteral{1},
                .if_branch = std::make_shared<Statement>(ExpressionStatement{StringLiteral{"true!"}}),
                .else_branch = std::make_shared<Statement>(ExpressionStatement{StringLiteral{"false!"}}),
        };

        Interpreter e;
        a.expect_eq(e.execute(if_stmt), Value{"true!"});

        if_stmt.test = NumericLiteral{0};
        a.expect_eq(e.execute(if_stmt), Value{"false!"});
    });

    s.add_test("native function", [](etest::IActions &a) {
        Interpreter e;

        std::string argument{};
        e.variables["set_string_and_get_42"] = Value{NativeFunction{[&](auto args) {
            a.require_eq(args.size(), std::size_t{1});
            argument = args[0].as_string();
            return Value{42};
        }}};

        auto call = CallExpression{
                .callee = std::make_shared<Expression>(Identifier{"set_string_and_get_42"}),
                .arguments{std::make_shared<Expression>(StringLiteral{"did it!"})},
        };

        a.expect_eq(e.execute(call), Value{42});
        a.expect_eq(argument, "did it!");
    });

    s.add_test("empty statement", [](etest::IActions &a) {
        Interpreter e;
        a.expect_eq(e.execute(EmptyStatement{}), Value{});
        a.expect(e.variables.empty());
    });

    s.add_test("while statement", [](etest::IActions &a) {
        Interpreter e;

        int loop_count{};
        e.variables["should_continue"] = Value{NativeFunction{[&](auto const &args) {
            a.expect_eq(args.size(), std::size_t{0});
            // TODO(robinlinden): We don't have bool values yet.
            return Value{++loop_count < 3 ? 1. : 0.};
        }}};

        auto while_loop = WhileStatement{
                .test = CallExpression{.callee = std::make_shared<Expression>(Identifier{"should_continue"})},
                .body = std::make_shared<Statement>(EmptyStatement{}),
        };

        a.expect_eq(e.execute(while_loop), Value{});
        a.expect_eq(loop_count, 3);
    });

    s.add_test("program", [](etest::IActions &a) {
        Program p{
                .body{
                        ExpressionStatement{StringLiteral{"hello"}},
                        ExpressionStatement{NumericLiteral{42.}},
                },
        };

        a.expect_eq(Interpreter{}.execute(p), Value{42.});
        a.expect_eq(Interpreter{}.execute(Program{}), Value{});
    });

    return s.run();
}
