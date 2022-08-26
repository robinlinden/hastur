// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef JS_AST_EXECUTOR_H_
#define JS_AST_EXECUTOR_H_

#include "js/ast.h"

#include <functional>
#include <map>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace js::ast {

class AstExecutor {
public:
    Value execute(auto const &ast) { return (*this)(ast); }

    Value operator()(Literal const &v) { return std::visit(*this, v); }
    Value operator()(NumericLiteral const &v) { return Value{v.value}; }
    Value operator()(StringLiteral const &v) { return Value{v.value}; }
    Value operator()(Expression const &v) { return std::visit(*this, v); }
    Value operator()(Identifier const &v) { return Value{v.name}; }
    Value operator()(Pattern const &v) { return std::visit(*this, v); }
    Value operator()(Declaration const &v) { return std::visit(*this, v); }
    Value operator()(Statement const &v) { return std::visit(*this, v); }

    Value operator()(ExpressionStatement const &v) { return execute(v.expression); }

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

    Value operator()(FunctionBody const &v) {
        for (auto const &statement : v.body) {
            execute(statement);
            if (returning) {
                return *std::exchange(returning, std::nullopt);
            }
        }

        return Value{};
    }

    Value operator()(ReturnStatement const &v) {
        returning = v.argument ? execute(*v.argument) : Value{};
        return Value{};
    }

    std::map<std::string, Value, std::less<>> variables;
    std::optional<Value> returning;
};

} // namespace js::ast

#endif
