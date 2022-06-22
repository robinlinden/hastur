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

// Based on
// https://github.com/estree/estree/blob/a965082b24524196232232ac75e3f80b17b28bc4/es5.md

namespace js {
namespace ast {

// TODO(robinlinden): This needs to support more values.
class Value {
public:
    explicit Value() : value_{} {}
    explicit Value(double value) : value_{value} {}
    explicit Value(std::string value) : value_{std::move(value)} {}

    // std::monostate -> undefined
    using ValueT = std::variant<std::monostate, std::string, double>;
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

class Literal : public Expression {
public:
    explicit Literal(Value value) : value_{std::move(value)} {}
    Value execute(Context &) const override { return value_; }

private:
    Value value_;
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
