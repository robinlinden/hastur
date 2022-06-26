// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "js/ast.h"

#include "etest/etest.h"

#include <cstddef>
#include <string>

using namespace js::ast;
using namespace std::literals;
using etest::expect;
using etest::expect_eq;

int main() {
    etest::test("literals", [] {
        js::ast::Context ctx;
        expect_eq(NumericLiteral{5.}.execute(ctx), Value{5.});
        expect_eq(StringLiteral{"hello"s}.execute(ctx), Value{"hello"s});
    });

    etest::test("variable declaration", [] {
        // AST for `var a = 1`
        auto init = std::make_unique<NumericLiteral>(1.);
        auto id = std::make_unique<Identifier>("a");
        auto declarator = VariableDeclarator{};
        declarator.id = std::move(id);
        declarator.init = std::move(init);

        auto declaration = std::make_unique<VariableDeclaration>();
        declaration->declarations.push_back(std::move(declarator));

        Program program;
        program.body.push_back(std::move(declaration));

        Context ctx;
        expect(program.execute(ctx).is_undefined());
        expect_eq(ctx, Context{.variables = {{"a", Value{1.}}}});
    });

    etest::test("binary expression", [] {
        auto plus_expr = BinaryExpression{
                BinaryOperator::Plus, std::make_unique<NumericLiteral>(11.), std::make_unique<NumericLiteral>(31.)};
        Context ctx;
        expect_eq(plus_expr.execute(ctx), Value{42.});

        auto minus_expr = BinaryExpression{
                BinaryOperator::Minus, std::make_unique<NumericLiteral>(11.), std::make_unique<NumericLiteral>(31.)};
        expect_eq(minus_expr.execute(ctx), Value{-20.});
    });

    etest::test("function declaration", [] {
        // AST for `function a() {}`
        auto declaration = FunctionDeclaration{Identifier{"a"}, {}, FunctionBody{{}}};
        Context ctx;
        expect(declaration.execute(ctx).is_undefined());
        expect_eq(ctx.variables.size(), std::size_t{1});

        // Check that we can call the declared function.
        ctx.variables["arguments"] = Value{std::vector<Value>{}};
        expect(ctx.variables.at("a").as_function()->execute(ctx).is_undefined());
    });

    etest::test("function call", [] {
        // AST for `function func(arg1, arg2) {}`
        std::vector<std::unique_ptr<Pattern>> params;
        params.push_back(std::make_unique<Identifier>("arg1"));
        params.push_back(std::make_unique<Identifier>("arg2"));
        auto declaration = FunctionDeclaration{Identifier{"func"}, std::move(params), FunctionBody{{}}};
        Context ctx;
        expect(declaration.execute(ctx).is_undefined());
        expect_eq(ctx.variables.size(), std::size_t{1});

        // AST for `func(1, "hello")`
        std::vector<std::unique_ptr<Expression>> args;
        args.push_back(std::make_unique<NumericLiteral>(1));
        args.push_back(std::make_unique<StringLiteral>("hello"));
        auto call = CallExpression{std::make_unique<Identifier>("func"), std::move(args)};
        expect(call.execute(ctx).is_undefined());

        // Abuse the fact that we don't yet support scopes to check that the
        // arguments were mapped to the correct names. This should break when
        // scopes are supported.
        expect_eq(ctx.variables.size(), std::size_t{4}); // func, arg1, arg2, arguments
        expect_eq(ctx.variables.at("arg1"), Value{1});
        expect_eq(ctx.variables.at("arg2"), Value{"hello"});
    });

    return etest::run_all_tests();
}
