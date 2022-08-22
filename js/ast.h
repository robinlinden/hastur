// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef JS_AST_H_
#define JS_AST_H_

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace js {
namespace ast {

struct Function;
struct BinaryExpression;
struct BlockStatement;
struct ReturnStatement;
struct CallExpression;
struct ExpressionStatement;
struct FunctionDeclaration;
struct Identifier;
struct NumericLiteral;
struct Program;
struct StringLiteral;
struct VariableDeclaration;
struct VariableDeclarator;

using Declaration = std::variant<FunctionDeclaration, VariableDeclaration>;
using Literal = std::variant<NumericLiteral, StringLiteral>;
using Pattern = std::variant<Identifier>;
using Statement = std::variant<Declaration, ExpressionStatement, BlockStatement, ReturnStatement>;
using Expression = std::variant<Identifier, Literal, CallExpression, BinaryExpression>;
using Node = std::variant<Expression, Statement, Pattern, Program, Function, VariableDeclarator>;

// TODO(robinlinden): This needs to support more values.
class Value {
public:
    explicit Value() : value_{Undefined{}} {}
    explicit Value(double value) : value_{value} {}
    explicit Value(std::string value) : value_{std::move(value)} {}
    explicit Value(std::shared_ptr<Function> value) : value_{std::move(value)} {}
    explicit Value(std::vector<Value> value) : value_{std::move(value)} {}

    bool is_undefined() const { return std::holds_alternative<Undefined>(value_); }
    bool is_number() const { return std::holds_alternative<double>(value_); }
    bool is_string() const { return std::holds_alternative<std::string>(value_); }
    bool is_function() const { return std::holds_alternative<std::shared_ptr<Function>>(value_); }
    bool is_vector() const { return std::holds_alternative<std::vector<Value>>(value_); }

    double as_number() const { return std::get<double>(value_); }
    std::string const &as_string() const { return std::get<std::string>(value_); }
    std::shared_ptr<Function const> as_function() const { return std::get<std::shared_ptr<Function>>(value_); }
    std::vector<Value> const &as_vector() const { return std::get<std::vector<Value>>(value_); }

    [[nodiscard]] bool operator==(Value const &) const = default;

private:
    struct Undefined {
        [[nodiscard]] bool operator==(Undefined const &) const = default;
    };
    std::variant<Undefined, std::string, double, std::shared_ptr<Function>, std::vector<Value>> value_;
};

struct NumericLiteral {
    double value{0.};
};

struct StringLiteral {
    std::string value;
};

struct Identifier {
    std::string name;
};

struct ExpressionStatement {
    std::shared_ptr<Expression> expression;
};

// TODO(robinlinden): Support more operators.
enum class BinaryOperator {
    Minus,
    Plus,
};

struct BinaryExpression {
    BinaryOperator op;
    std::shared_ptr<Expression> lhs;
    std::shared_ptr<Expression> rhs;
};

struct Program {
    std::vector<std::shared_ptr<Statement>> body;
};

struct BlockStatement {
    std::vector<std::shared_ptr<Statement>> body;
};

struct FunctionBody {
    std::vector<Statement> body;
};

struct Function {
    std::vector<Pattern> params;
    FunctionBody body;
};

struct FunctionDeclaration {
    Identifier id;
    std::shared_ptr<Function> function;
};

struct CallExpression {
    std::shared_ptr<Expression> callee;
    std::vector<std::shared_ptr<Expression>> arguments;
};

struct VariableDeclarator {
    Pattern id;
    std::optional<Expression> init;
};

struct VariableDeclaration {
    std::vector<VariableDeclarator> declarations;
    enum class Kind {
        Var,
    };
    Kind kind{Kind::Var};
};

struct ReturnStatement {
    std::optional<Expression> argument;
};

} // namespace ast
} // namespace js

#endif
