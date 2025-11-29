// SPDX-FileCopyrightText: 2022-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "js/interpreter.h"

#include "js/ast.h"

#include "etest/etest2.h"

#include <tl/expected.hpp>

#include <cstddef>
#include <memory>
#include <string>
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

    s.add_test("binary expression, plus, exception in lhs", [](etest::IActions &a) {
        // foo() + 31
        auto plus_expr = BinaryExpression{
                .op = BinaryOperator::Plus,
                .lhs = std::make_shared<Expression>(CallExpression{
                        .callee = std::make_shared<Expression>(Identifier{"foo"}),
                }),
                .rhs = std::make_shared<Expression>(NumericLiteral{31.}),
        };

        Interpreter e;
        auto result = e.execute(plus_expr);
        a.expect_eq(result.has_value(), false);
    });

    s.add_test("binary expression, plus, exception in rhs", [](etest::IActions &a) {
        // 11 + foo()
        auto plus_expr = BinaryExpression{
                .op = BinaryOperator::Plus,
                .lhs = std::make_shared<Expression>(NumericLiteral{11.}),
                .rhs = std::make_shared<Expression>(CallExpression{
                        .callee = std::make_shared<Expression>(Identifier{"foo"}),
                }),
        };

        Interpreter e;
        auto result = e.execute(plus_expr);
        a.expect_eq(result.has_value(), false);
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

    s.add_test("variable declaration, exception in init", [](etest::IActions &a) {
        // var a = foo()
        auto declaration = VariableDeclaration{{
                VariableDeclarator{
                        .id = Identifier{"a"},
                        .init = CallExpression{.callee = std::make_shared<Expression>(Identifier{"foo"})},
                },
        }};

        Interpreter e;
        auto result = e.execute(declaration);
        a.expect_eq(result.has_value(), false);
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

        // And check that we can resolve function arguments via variables.
        e.variables["a"] = Value{38.};
        call = CallExpression{
                .callee = std::make_shared<Expression>(Identifier{"func"}),
                .arguments{
                        std::make_shared<Expression>(Identifier{"a"}),
                        std::make_shared<Expression>(NumericLiteral{4.}),
                },
        };

        a.expect_eq(e.execute(call), Value{38. + 4.});
    });

    s.add_test("function call, exception in body", [](etest::IActions &a) {
        auto function_body = ReturnStatement{CallExpression{
                .callee = std::make_shared<Expression>(Identifier{"will_throw"}),
        }};

        auto declaration = FunctionDeclaration{
                .id = Identifier{"func"},
                .function = std::make_shared<Function>(Function{
                        .params{},
                        .body{{std::move(function_body)}},
                }),
        };

        auto call = CallExpression{.callee = std::make_shared<Expression>(Identifier{"func"})};

        Interpreter e;
        a.expect_eq(e.execute(declaration), Value{});
        auto result = e.execute(call);
        a.expect_eq(result.has_value(), false);
    });

    s.add_test("function call, not found", [](etest::IActions &a) {
        auto call = CallExpression{
                .callee = std::make_shared<Expression>(Identifier{"does_not_exist"}),
        };

        auto result = Interpreter{}.execute(call);
        a.expect_eq(result.has_value(), false);
    });

    s.add_test("function call, not a function", [](etest::IActions &a) {
        auto call = CallExpression{
                .callee = std::make_shared<Expression>(Identifier{"not_a_function"}),
        };

        Interpreter e;
        e.variables["not_a_function"] = Value{42.};

        auto result = e.execute(call);
        a.expect_eq(result.has_value(), false);
    });

    s.add_test("function call, exception in callee", [](etest::IActions &a) {
        // foo()()
        auto call = CallExpression{
                .callee = std::make_shared<Expression>(CallExpression{
                        .callee = std::make_shared<Expression>(Identifier{"foo"}),
                }),
        };

        Interpreter e;
        auto result = e.execute(call);
        a.expect_eq(result.has_value(), false);
    });

    s.add_test("function call, exception in argument", [](etest::IActions &a) {
        auto call = CallExpression{
                .callee = std::make_shared<Expression>(Identifier{"func"}),
                .arguments{
                        std::make_shared<Expression>(Identifier{"will_throw"}),
                },
        };

        Interpreter e;
        e.variables["func"] = Value{NativeFunction{[](auto const &) { return Value{}; }}};

        auto result = e.execute(call);
        a.expect_eq(result.has_value(), false);
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

    s.add_test("member expression, object not found", [](etest::IActions &a) {
        auto member_expr = MemberExpression{
                .object = std::make_shared<Expression>(Identifier{"does_not_exist"}),
                .property = Identifier{"hello"},
        };

        Interpreter e;
        auto result = e.execute(member_expr);
        a.expect_eq(result.has_value(), false);
    });

    s.add_test("member expression, property not found", [](etest::IActions &a) {
        auto member_expr = MemberExpression{
                .object = std::make_shared<Expression>(Identifier{"obj"}),
                .property = Identifier{"does_not_exist"},
        };

        Interpreter e;
        e.variables["obj"] = Value{Object{{"hello", Value{5.}}}};
        auto result = e.execute(member_expr);
        a.expect_eq(result.has_value(), false);
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

    s.add_test("assignment expression", [](etest::IActions &a) {
        auto assign_expr = AssignmentExpression{
                .left = std::make_shared<Expression>(Identifier{"somevar"}),
                .right = std::make_shared<Expression>(NumericLiteral{55.}),
        };

        Interpreter e;
        a.expect_eq(e.execute(assign_expr), Value{55.});
        a.expect_eq(e.variables, decltype(e.variables){{"somevar", Value{55.}}});
    });

    s.add_test("assignment expression, exception in right-hand side", [](etest::IActions &a) {
        auto assign_expr = AssignmentExpression{
                .left = std::make_shared<Expression>(Identifier{"somevar"}),
                .right = std::make_shared<Expression>(Identifier{"blargh"}),
        };

        Interpreter e;
        auto result = e.execute(assign_expr);
        a.expect_eq(result.has_value(), false);
        a.expect(e.variables.find("somevar") == e.variables.end());
    });

    s.add_test("if, exception in test", [](etest::IActions &a) {
        auto if_stmt = IfStatement{
                .test = CallExpression{.callee = std::make_shared<Expression>(Identifier{"foo"})},
                .if_branch = std::make_shared<Statement>(ExpressionStatement{StringLiteral{"true!"}}),
        };

        Interpreter e;
        auto result = e.execute(if_stmt);
        a.expect_eq(result.has_value(), false);
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

        std::string argument;
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

    s.add_test("native function, exception from native code", [](etest::IActions &a) {
        Interpreter e;
        e.variables["will_throw"] = Value{NativeFunction{[](auto const &) {
            return tl::unexpected{js::ast::ErrorValue{js::ast::Value{"Bad!"}}}; //
        }}};

        auto call = CallExpression{
                .callee = std::make_shared<Expression>(Identifier{"will_throw"}),
        };

        auto result = e.execute(call);
        a.expect_eq(result, tl::unexpected{js::ast::ErrorValue{js::ast::Value{"Bad!"}}});
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

    s.add_test("while statement, exception in test", [](etest::IActions &a) {
        Interpreter e;

        auto while_loop = WhileStatement{
                .test = CallExpression{.callee = std::make_shared<Expression>(Identifier{"will_throw"})},
                .body = std::make_shared<Statement>(EmptyStatement{}),
        };

        auto result = e.execute(while_loop);
        a.expect_eq(result.has_value(), false);
    });

    s.add_test("while statement, exception in body", [](etest::IActions &a) {
        Interpreter e;

        int loop_count{};
        e.variables["should_continue"] = Value{NativeFunction{[&](auto const &) {
            // TODO(robinlinden): We don't have bool values yet.
            return Value{++loop_count < 3 ? 1. : 0.};
        }}};

        auto while_loop = WhileStatement{
                .test = CallExpression{.callee = std::make_shared<Expression>(Identifier{"should_continue"})},
                .body = std::make_shared<Statement>(ExpressionStatement{
                        CallExpression{.callee = std::make_shared<Expression>(Identifier{"will_throw"})},
                }),
        };

        auto result = e.execute(while_loop);
        a.expect_eq(result.has_value(), false);
        a.expect_eq(loop_count, 1);
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

    s.add_test("program, exception", [](etest::IActions &a) {
        Program p{
                .body{
                        ExpressionStatement{CallExpression{.callee = std::make_shared<Expression>(Identifier{"foo"})}},
                        ExpressionStatement{NumericLiteral{42.}},
                },
        };

        auto result = Interpreter{}.execute(p);
        a.expect_eq(result.has_value(), false);
    });

    s.add_test("block statement", [](etest::IActions &a) {
        BlockStatement block{
                .body{
                        ExpressionStatement{StringLiteral{"hello"}},
                        ExpressionStatement{NumericLiteral{42.}},
                },
        };

        a.expect_eq(Interpreter{}.execute(block), Value{42.});
        a.expect_eq(Interpreter{}.execute(BlockStatement{}), Value{});
    });

    s.add_test("block statement, exception", [](etest::IActions &a) {
        BlockStatement block{
                .body{
                        ExpressionStatement{CallExpression{.callee = std::make_shared<Expression>(Identifier{"foo"})}},
                        ExpressionStatement{NumericLiteral{42.}},
                },
        };

        auto result = Interpreter{}.execute(block);
        a.expect_eq(result.has_value(), false);
    });

    return s.run();
}
