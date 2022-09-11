// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "js/ast_executor.h"

#include "js/ast.h"

#include "etest/etest.h"

#include <tuple>

using namespace js::ast;
using etest::expect_eq;

int main() {
    etest::test("literals", [] {
        AstExecutor e;
        expect_eq(e.execute(NumericLiteral{5.}), Value{5.});
        expect_eq(e.execute(StringLiteral{"hello"}), Value{"hello"});
    });

    etest::test("binary expression, plus", [] {
        auto plus_expr = BinaryExpression{
                .op = BinaryOperator::Plus,
                .lhs = std::make_shared<Expression>(NumericLiteral{11.}),
                .rhs = std::make_shared<Expression>(NumericLiteral{31.}),
        };

        AstExecutor e;
        expect_eq(e.execute(plus_expr), Value{42.});
    });

    etest::test("binary expression, identifiers", [] {
        auto plus_expr = BinaryExpression{
                .op = BinaryOperator::Plus,
                .lhs = std::make_shared<Expression>(Identifier{"eleven"}),
                .rhs = std::make_shared<Expression>(Identifier{"thirtyone"}),
        };

        AstExecutor e;
        e.variables["eleven"] = Value{11.};
        e.variables["thirtyone"] = Value{31.};
        expect_eq(e.execute(plus_expr), Value{42.});
    });

    etest::test("binary expression, minus", [] {
        auto minus_expr = BinaryExpression{
                .op = BinaryOperator::Minus,
                .lhs = std::make_shared<Expression>(NumericLiteral{11.}),
                .rhs = std::make_shared<Expression>(NumericLiteral{31.}),
        };

        AstExecutor e;
        expect_eq(e.execute(minus_expr), Value{-20.});
    });

    etest::test("the ast is copyable", [] {
        Program p1;
        auto p2 = p1;
        std::ignore = p2;
    });

    etest::test("variable declaration", [] {
        auto declaration = VariableDeclaration{{
                VariableDeclarator{
                        .id = Identifier{"a"},
                        .init = NumericLiteral{1.},
                },
        }};

        AstExecutor e;
        expect_eq(e.execute(declaration), Value{});
        expect_eq(e.variables, decltype(e.variables){{"a", Value{1.}}});
    });

    etest::test("function call, arguments", [] {
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

        AstExecutor e;
        expect_eq(e.execute(declaration), Value{});
        expect_eq(e.execute(call), Value{13. + 4.});
    });

    etest::test("return, values are returned", [] {
        auto declaration = FunctionDeclaration{
                .id = Identifier{"func"},
                .function = std::make_shared<Function>(Function{
                        .params{},
                        .body{{ReturnStatement{NumericLiteral{42.}}}},
                }),
        };

        auto call = CallExpression{.callee = std::make_shared<Expression>(Identifier{"func"})};

        AstExecutor e;
        expect_eq(e.execute(declaration), Value{});
        expect_eq(e.execute(call), Value{42.});
    });

    etest::test("return, function execution is ended", [] {
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

        AstExecutor e;
        expect_eq(e.execute(declaration), Value{});
        expect_eq(e.execute(call), Value{});
    });

    etest::test("expression statement", [] {
        AstExecutor e;
        expect_eq(e.execute(ExpressionStatement{StringLiteral{"hi"}}), Value{"hi"});
        expect_eq(e.execute(ExpressionStatement{NumericLiteral{1213}}), Value{1213});
    });

    etest::test("if", [] {
        auto if_stmt = IfStatement{
                .test = NumericLiteral{1},
                .if_branch = std::make_shared<Statement>(ExpressionStatement{StringLiteral{"true!"}}),
        };

        AstExecutor e;
        expect_eq(e.execute(if_stmt), Value{"true!"});

        if_stmt.test = NumericLiteral{0};
        expect_eq(e.execute(if_stmt), Value{});
    });

    etest::test("if-else", [] {
        auto if_stmt = IfStatement{
                .test = NumericLiteral{1},
                .if_branch = std::make_shared<Statement>(ExpressionStatement{StringLiteral{"true!"}}),
                .else_branch = std::make_shared<Statement>(ExpressionStatement{StringLiteral{"false!"}}),
        };

        AstExecutor e;
        expect_eq(e.execute(if_stmt), Value{"true!"});

        if_stmt.test = NumericLiteral{0};
        expect_eq(e.execute(if_stmt), Value{"false!"});
    });

    return etest::run_all_tests();
}
