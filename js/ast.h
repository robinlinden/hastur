// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef JS_AST_H_
#define JS_AST_H_

#include <map>
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

using FunctionBody = BlockStatement;

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

class AstExecutor {
public:
    Value execute(auto const &ast) { return (*this)(ast); }

    Value operator()(Literal const &v) { return std::visit(*this, v); }
    Value operator()(NumericLiteral const &v) { return Value{v.value}; }
    Value operator()(StringLiteral const &v) { return Value{v.value}; }
    Value operator()(Expression const &v) { return std::visit(*this, v); }
    Value operator()(Identifier const &v) { return Value{v.name}; }
    Value operator()(Pattern const &v) { return std::visit(*this, v); }

    Value operator()(BinaryExpression const &v) {
        auto lhs = execute(*v.lhs);
        auto rhs = execute(*v.rhs);
        switch (v.op) {
            case BinaryOperator::Plus:
                return Value{lhs.as_number() + rhs.as_number()};
            case BinaryOperator::Minus:
                return Value{lhs.as_number() - rhs.as_number()};
        }
        std::abort();
    }

    Value operator()(VariableDeclaration const &v) {
        for (auto const &declaration : v.declarations) {
            execute(declaration);
        }

        return Value{};
    }

    Value operator()(VariableDeclarator const &v) {
        auto name = execute(v.id).as_string();
        variables[name] = v.init ? execute(*v.init) : Value{};
        return Value{};
    }

    Value operator()(FunctionDeclaration const &v) {
        variables[v.id.name] = Value{v.function};
        return Value{};
    }

    Value operator()(CallExpression const &v) {
        auto const &fn = *variables.at(execute(*v.callee).as_string()).as_function();

        std::vector<Value> args;
        args.reserve(v.arguments.size());
        for (auto const &arg : v.arguments) {
            args.push_back(execute(*arg));
        }

        // https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Functions/arguments
        variables["arguments"] = Value{std::move(args)};
        return execute(fn);
    }

    Value operator()(Function const &v) {
        auto const &args = variables.at("arguments").as_vector();
        for (std::size_t i = 0; i < v.params.size(); ++i) {
            auto id = execute(v.params[i]).as_string();
            variables[std::move(id)] = i < args.size() ? args[i] : Value{};
        }

        return execute(v.body);
    }

    Value operator()(BlockStatement const &) {
        // TODO(robinlinden): Implement.
        return Value{};
    }

    std::map<std::string, Value, std::less<>> variables;
};

} // namespace ast
} // namespace js

#endif
