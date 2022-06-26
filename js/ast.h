// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef JS_AST_H_
#define JS_AST_H_

#include <cstdlib>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

// Based on
// https://github.com/estree/estree/blob/a965082b24524196232232ac75e3f80b17b28bc4/es5.md

namespace js {
namespace ast {

// TODO(robinlinden): This needs to support more values.
class Function;
class Value {
public:
    explicit Value() : value_{} {}
    explicit Value(double value) : value_{value} {}
    explicit Value(std::string value) : value_{std::move(value)} {}
    explicit Value(std::shared_ptr<Function> value) : value_{std::move(value)} {}

    // std::monostate -> undefined
    using ValueT = std::variant<std::monostate, std::string, double, std::shared_ptr<Function>>;
    [[nodiscard]] ValueT const &value() const { return value_; }

    [[nodiscard]] bool operator==(Value const &) const = default;

private:
    ValueT value_;
};

struct Context {
    [[nodiscard]] bool operator==(Context const &) const = default;
    std::map<std::string, Value> variables;
};

class Node {
public:
    virtual ~Node() = default;
    virtual Value execute(Context &) const = 0;
};

class Expression : public Node {};
class Statement : public Node {};
class Pattern : public Node {};
class Declaration : public Statement {};

class Identifier : public Expression, public Pattern {
public:
    explicit Identifier(std::string name) : name_{std::move(name)} {}
    Value execute(Context &) const override { return Value{name_}; }

private:
    std::string name_;
};

class Literal : public Expression {};

class NumericLiteral : public Literal {
public:
    explicit NumericLiteral(double value) : value_{value} {}
    Value execute(Context &) const override { return Value{value_}; }

private:
    double value_{};
};

class StringLiteral : public Literal {
public:
    explicit StringLiteral(std::string value) : value_{std::move(value)} {}
    Value execute(Context &) const override { return Value{value_}; }

private:
    std::string value_{};
};

// TODO(robinlinden): Support more operators.
enum class BinaryOperator {
    Minus,
    Plus,
};

class BinaryExpression : public Expression {
public:
    BinaryExpression(BinaryOperator op, std::unique_ptr<Expression> left, std::unique_ptr<Expression> right)
        : op_{op}, left_{std::move(left)}, right_{std::move(right)} {}

    // TODO(robinlinden): Support values that aren't doubles.
    Value execute(Context &ctx) const override {
        auto lhs = left_->execute(ctx);
        auto rhs = right_->execute(ctx);
        switch (op_) {
            case BinaryOperator::Plus:
                return Value{std::get<double>(lhs.value()) + std::get<double>(rhs.value())};
            case BinaryOperator::Minus:
                return Value{std::get<double>(lhs.value()) - std::get<double>(rhs.value())};
        }
        std::abort();
    }

private:
    BinaryOperator op_{};
    std::unique_ptr<Expression> left_;
    std::unique_ptr<Expression> right_;
};

class Program : public Node {
public:
    Value execute(Context &ctx) const override {
        Value ret{};
        for (auto const &statement : body) {
            ret = statement->execute(ctx);
        }
        return ret;
    }

    std::vector<std::unique_ptr<Statement>> body;
};

class BlockStatement : public Statement {
public:
    explicit BlockStatement(std::vector<std::unique_ptr<Statement>> body) : body_{std::move(body)} {}

    Value execute(Context &ctx) const override {
        Value ret;
        for (auto const &statement : body_) {
            ret = statement->execute(ctx);
        }
        return ret;
    }

private:
    std::vector<std::unique_ptr<Statement>> body_;
};

class FunctionBody : public BlockStatement {
public:
    using BlockStatement::BlockStatement;
    using BlockStatement::execute;
};

class Function : public Node {
public:
    Function(std::vector<std::unique_ptr<Pattern>> params, FunctionBody body)
        : params_{std::move(params)}, body_{std::move(body)} {}

    Value execute(Context &ctx) const override {
        // TODO(robinlinden): Push scope to context, add params as variables.
        return body_.execute(ctx);
    }

private:
    std::vector<std::unique_ptr<Pattern>> params_;
    FunctionBody body_;
};

// TODO(robinlinden): es5.md says that this should inherit from Function, but
// that is weird.
class FunctionDeclaration : public Declaration {
public:
    FunctionDeclaration(Identifier id, std::vector<std::unique_ptr<Pattern>> params, FunctionBody body)
        : id_{std::move(id)}, function_{std::make_shared<Function>(std::move(params), std::move(body))} {}

    Value execute(Context &ctx) const override {
        ctx.variables[std::get<std::string>(id_.execute(ctx).value())] = Value{function_};
        return Value{};
    }

private:
    Identifier id_;
    std::shared_ptr<Function> function_;
};

class VariableDeclarator : public Node {
public:
    Value execute(Context &ctx) const override {
        auto name = std::get<std::string>(id->execute(ctx).value());
        ctx.variables[std::move(name)] = init ? (*init)->execute(ctx) : Value{};
        return Value{};
    }

    std::unique_ptr<Pattern> id;
    std::optional<std::unique_ptr<Expression>> init;
};

class VariableDeclaration : public Declaration {
public:
    Value execute(Context &ctx) const override {
        for (auto const &declaration : declarations) {
            declaration.execute(ctx);
        }

        return Value{};
    }

    std::vector<VariableDeclarator> declarations;
    enum class Kind {
        Var,
    };
    Kind kind{Kind::Var};
};

} // namespace ast
} // namespace js

#endif
