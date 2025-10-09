// SPDX-FileCopyrightText: 2022-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef JS_AST_H_
#define JS_AST_H_

#include <tl/expected.hpp>

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace js::ast {

class Value;
struct NativeFunction;
struct Function;
struct BinaryExpression;
struct BlockStatement;
struct ReturnStatement;
struct CallExpression;
struct MemberExpression;
struct ExpressionStatement;
struct FunctionDeclaration;
struct Identifier;
struct IfStatement;
struct NumericLiteral;
struct Program;
struct StringLiteral;
struct VariableDeclaration;
struct VariableDeclarator;
struct EmptyStatement;
struct WhileStatement;

using Declaration = std::variant<FunctionDeclaration, VariableDeclaration>;
using Literal = std::variant<NumericLiteral, StringLiteral>;
using Pattern = std::variant<Identifier>;
using Statement = std::variant<Declaration,
        ExpressionStatement,
        BlockStatement,
        ReturnStatement,
        IfStatement,
        WhileStatement,
        EmptyStatement>;
using Expression = std::variant<Identifier, Literal, CallExpression, MemberExpression, BinaryExpression>;

struct ErrorValue;
using ValueOrException = tl::expected<Value, ErrorValue>;

struct NativeFunction {
    std::function<ValueOrException(std::vector<Value> const &)> f;
    [[nodiscard]] bool operator==(NativeFunction const &) const { return false; }
};

using Object = std::map<std::string, Value, std::less<>>;

// TODO(robinlinden): This needs to support more values.
class Value {
public:
    explicit Value() : value_{Undefined{}} {}
    explicit Value(double value) : value_{value} {}
    explicit Value(std::string value) : value_{std::move(value)} {}
    explicit Value(std::shared_ptr<Function> value) : value_{std::move(value)} {}
    explicit Value(std::vector<Value> value) : value_{std::move(value)} {}
    explicit Value(Object value) : value_{std::move(value)} {}
    explicit Value(NativeFunction value) : value_{std::move(value)} {}
    explicit Value(decltype(NativeFunction::f) f) : value_{NativeFunction{std::move(f)}} {}

    bool is_undefined() const { return std::holds_alternative<Undefined>(value_); }
    bool is_number() const { return std::holds_alternative<double>(value_); }
    bool is_string() const { return std::holds_alternative<std::string>(value_); }
    bool is_function() const { return std::holds_alternative<std::shared_ptr<Function>>(value_); }
    bool is_vector() const { return std::holds_alternative<std::vector<Value>>(value_); }
    bool is_object() const { return std::holds_alternative<Object>(value_); }
    bool is_native_function() const { return std::holds_alternative<NativeFunction>(value_); }

    double as_number() const { return std::get<double>(value_); }
    std::string const &as_string() const { return std::get<std::string>(value_); }
    std::shared_ptr<Function const> as_function() const { return std::get<std::shared_ptr<Function>>(value_); }
    std::vector<Value> const &as_vector() const { return std::get<std::vector<Value>>(value_); }
    Object const &as_object() const { return std::get<Object>(value_); }
    NativeFunction const &as_native_function() const { return std::get<NativeFunction>(value_); }

    bool as_bool() const {
        // TODO(robinlinden): false, 0n, null, NaN, objects with an [[IsHTMLDDA]] internal slot.
        // https://developer.mozilla.org/en-US/docs/Glossary/Falsy
        bool is_false = *this == Value{0} || *this == Value{-0} || *this == Value{""} || *this == Value{};
        return !is_false;
    }

    [[nodiscard]] bool operator==(Value const &) const = default;

private:
    struct Undefined {
        [[nodiscard]] bool operator==(Undefined const &) const = default;
    };

    std::variant<Undefined,
            std::string,
            double,
            std::shared_ptr<Function>,
            std::vector<Value>,
            Object,
            NativeFunction //
            >
            value_;
};

struct ErrorValue {
    Value e;
    [[nodiscard]] bool operator==(ErrorValue const &) const = default;
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

// TODO(robinlinden): Support more operators.
enum class BinaryOperator : std::uint8_t {
    Minus,
    Plus,
};

struct BinaryExpression {
    BinaryOperator op;
    std::shared_ptr<Expression> lhs;
    std::shared_ptr<Expression> rhs;
};

struct Program {
    std::vector<Statement> body;
};

struct BlockStatement {
    std::vector<Statement> body;
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

struct MemberExpression {
    std::shared_ptr<Expression> object;
    Identifier property;
};

struct VariableDeclarator {
    Pattern id;
    std::optional<Expression> init;
};

struct VariableDeclaration {
    std::vector<VariableDeclarator> declarations;
    enum class Kind : std::uint8_t {
        Var,
    };
    Kind kind{Kind::Var};
};

struct ExpressionStatement {
    Expression expression;
};

struct ReturnStatement {
    std::optional<Expression> argument;
};

struct IfStatement {
    Expression test;
    std::shared_ptr<Statement> if_branch;
    std::optional<std::shared_ptr<Statement>> else_branch;
};

struct EmptyStatement {};

struct WhileStatement {
    Expression test;
    std::shared_ptr<Statement> body;
};

} // namespace js::ast

#endif
