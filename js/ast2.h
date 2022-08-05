// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef JS_AST2_H_
#define JS_AST2_H_

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace js {
namespace ast2 {

class Function;
class BinaryExpression;
class BlockStatement;
class CallExpression;
class ExpressionStatement;
class FunctionDeclaration;
class Identifier;
class NumericLiteral;
class Program;
class StringLiteral;
class VariableDeclaration;
class VariableDeclarator;

using Declaration = std::variant<FunctionDeclaration, VariableDeclaration>;
using Literal = std::variant<NumericLiteral, StringLiteral>;
using Pattern = std::variant<Identifier>;
using Statement = std::variant<Declaration, ExpressionStatement, BlockStatement>;
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

class NumericLiteral {
public:
    explicit NumericLiteral(double value) : value_{value} {}
    [[nodiscard]] double value() const { return value_; }

private:
    double value_{};
};

class StringLiteral {
public:
    explicit StringLiteral(std::string value) : value_{std::move(value)} {}
    [[nodiscard]] std::string const &value() const { return value_; }

private:
    std::string value_{};
};

class Identifier {
public:
    explicit Identifier(std::string name) : name_{std::move(name)} {}

private:
    std::string name_;
};

class ExpressionStatement {
public:
    explicit ExpressionStatement(std::shared_ptr<Expression> expression) : expression_{std::move(expression)} {}

private:
    std::shared_ptr<Expression> expression_;
};

// TODO(robinlinden): Support more operators.
enum class BinaryOperator {
    Minus,
    Plus,
};

class BinaryExpression {
public:
    BinaryExpression(BinaryOperator op, std::shared_ptr<Expression> left, std::shared_ptr<Expression> right)
        : op_{op}, left_{std::move(left)}, right_{std::move(right)} {}

    BinaryOperator op() const { return op_; }
    std::shared_ptr<Expression> const &lhs() const { return left_; }
    std::shared_ptr<Expression> const &rhs() const { return right_; }

private:
    BinaryOperator op_;
    std::shared_ptr<Expression> left_;
    std::shared_ptr<Expression> right_;
};

class Program {
public:
    std::vector<std::shared_ptr<Statement>> body;
};

class BlockStatement {
public:
    explicit BlockStatement(std::vector<std::shared_ptr<Statement>> body) : body_{std::move(body)} {}

private:
    std::vector<std::shared_ptr<Statement>> body_;
};

using FunctionBody = BlockStatement;

class Function {
public:
    Function(std::vector<Pattern> params, FunctionBody body) : params_{std::move(params)}, body_{std::move(body)} {}

private:
    std::vector<Pattern> params_;
    FunctionBody body_;
};

class FunctionDeclaration {
public:
    FunctionDeclaration(Identifier id, std::vector<Pattern> params, FunctionBody body)
        : id_{std::move(id)}, function_{std::make_shared<Function>(std::move(params), std::move(body))} {}

private:
    Identifier id_;
    std::shared_ptr<Function> function_;
};

class CallExpression {
public:
    CallExpression(std::shared_ptr<Expression> callee, std::vector<std::shared_ptr<Expression>> arguments)
        : callee_{std::move(callee)}, arguments_{std::move(arguments)} {}

private:
    std::shared_ptr<Expression> callee_;
    std::vector<std::shared_ptr<Expression>> arguments_;
};

class VariableDeclarator {
public:
    Pattern id;
    std::optional<Expression> init;
};

class VariableDeclaration {
public:
    std::vector<VariableDeclarator> declarations;
    enum class Kind {
        Var,
    };
    Kind kind{Kind::Var};
};

class AstExecutor {
public:
    Value execute(auto const &ast) { return (*this)(ast); }

    Value operator()(Literal const &v) { return std::visit(*this, v); }
    Value operator()(NumericLiteral const &v) { return Value{v.value()}; }
    Value operator()(StringLiteral const &v) { return Value{v.value()}; }
    Value operator()(Expression const &v) { return std::visit(*this, v); }
    Value operator()(Identifier const &) { std::abort(); }
    Value operator()(CallExpression const &) { std::abort(); }

    Value operator()(BinaryExpression const &v) {
        auto lhs = execute(*v.lhs());
        auto rhs = execute(*v.rhs());
        switch (v.op()) {
            case BinaryOperator::Plus:
                return Value{lhs.as_number() + rhs.as_number()};
            case BinaryOperator::Minus:
                return Value{lhs.as_number() - rhs.as_number()};
        }
        std::abort();
    }
};

} // namespace ast2
} // namespace js

#endif
