// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

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

    etest::test("function call", [] {
        auto declaration = FunctionDeclaration{
                .id = Identifier{"func"},
                .function = std::make_shared<Function>(Function{
                        .params{
                                Identifier{"one"},
                                Identifier{"two"},
                        },
                        .body{},
                }),
        };

        auto call = CallExpression{
                .callee = std::make_shared<Expression>(Identifier{"func"}),
                .arguments{
                        std::make_shared<Expression>(NumericLiteral{13.}),
                        std::make_shared<Expression>(StringLiteral{"beep beep boop"}),
                },
        };

        AstExecutor e;
        expect_eq(e.execute(declaration), Value{});
        expect_eq(e.execute(call), Value{});

        // Abuse the fact that we don't yet support scopes to check that the
        // arguments were mapped to the correct names. This should break when
        // scopes are supported.
        expect_eq(e.variables.at("one"), Value{13.});
        expect_eq(e.variables.at("two"), Value{"beep beep boop"});
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

    return etest::run_all_tests();
}
