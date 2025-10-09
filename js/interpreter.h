// SPDX-FileCopyrightText: 2022-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef JS_INTERPRETER_H_
#define JS_INTERPRETER_H_

#include "js/ast.h"

#include <tl/expected.hpp>

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace js::ast {

class Interpreter {
public:
    ValueOrException execute(auto const &ast) { return (*this)(ast); }

    ValueOrException operator()(Program const &v) {
        ValueOrException result{};

        for (auto const &statement : v.body) {
            result = execute(statement);
            if (!result) {
                return result;
            }
        }

        return result;
    }

    ValueOrException operator()(Literal const &v) { return std::visit(*this, v); }
    ValueOrException operator()(NumericLiteral const &v) { return Value{v.value}; }
    ValueOrException operator()(StringLiteral const &v) { return Value{v.value}; }
    ValueOrException operator()(Expression const &v) { return std::visit(*this, v); }
    ValueOrException operator()(Identifier const &v) { return Value{v.name}; }
    ValueOrException operator()(Pattern const &v) { return std::visit(*this, v); }
    ValueOrException operator()(Declaration const &v) { return std::visit(*this, v); }
    ValueOrException operator()(Statement const &v) { return std::visit(*this, v); }

    ValueOrException operator()(ExpressionStatement const &v) { return execute(v.expression); }

    ValueOrException operator()(BinaryExpression const &v) {
        auto lhs = get_value_resolving_variables(*v.lhs);
        if (!lhs) {
            return lhs;
        }

        auto rhs = get_value_resolving_variables(*v.rhs);
        if (!rhs) {
            return rhs;
        }

        switch (v.op) {
            case BinaryOperator::Plus:
                return Value{lhs->as_number() + rhs->as_number()};
            case BinaryOperator::Minus:
                return Value{lhs->as_number() - rhs->as_number()};
        }
        std::abort();
    }

    ValueOrException operator()(VariableDeclaration const &v) {
        for (auto const &declaration : v.declarations) {
            if (auto result = execute(declaration); !result) {
                return result;
            }
        }

        return Value{};
    }

    ValueOrException operator()(VariableDeclarator const &v) {
        auto maybe_name = execute(v.id);
        assert(maybe_name);

        auto name = maybe_name->as_string();
        ValueOrException init_value = v.init ? execute(*v.init) : Value{};
        if (!init_value) {
            return init_value;
        }

        variables[name] = *std::move(init_value);
        return Value{};
    }

    ValueOrException operator()(FunctionDeclaration const &v) {
        variables[v.id.name] = Value{v.function};
        return Value{};
    }

    ValueOrException operator()(CallExpression const &v) {
        Interpreter scope{*this};

        auto callee = execute(*v.callee);
        if (!callee) {
            return callee;
        }

        auto maybe_fn = variables.find(callee->as_string());
        if (maybe_fn == variables.end()) {
            // TODO(robinlinden): Better error value.
            return tl::unexpected{ErrorValue{Value{}}};
        }

        auto const &fn = maybe_fn->second;
        assert(fn.is_function() || fn.is_native_function());

        std::vector<Value> args;
        args.reserve(v.arguments.size());
        for (auto const &arg : v.arguments) {
            auto arg_value = get_value_resolving_variables(*arg);
            if (!arg_value) {
                return arg_value;
            }

            args.push_back(*std::move(arg_value));
        }

        // https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Functions/arguments
        scope.variables["arguments"] = Value{std::move(args)};
        if (fn.is_function()) {
            return scope.execute(*fn.as_function());
        }

        return scope.execute(fn.as_native_function());
    }

    ValueOrException operator()(MemberExpression const &v) {
        auto object = get_value_resolving_variables(*v.object);
        if (!object) {
            return object;
        }

        auto property = execute(v.property);
        assert(property);

        auto const &obj = object->as_object();
        auto it = obj.find(property->as_string());
        if (it == obj.end()) {
            // TODO(robinlinden): Better error value.
            return tl::unexpected{ErrorValue{Value{}}};
        }

        return it->second;
    }

    ValueOrException operator()(Function const &v) {
        auto const &args = variables.at("arguments").as_vector();
        for (std::size_t i = 0; i < v.params.size(); ++i) {
            auto maybe_id = execute(v.params[i]);
            assert(maybe_id);

            auto id = maybe_id->as_string();
            variables[std::move(id)] = i < args.size() ? args[i] : Value{};
        }

        return execute(v.body);
    }

    ValueOrException operator()(BlockStatement const &v) {
        ValueOrException result{};

        for (auto const &statement : v.body) {
            result = execute(statement);
            if (!result) {
                return result;
            }
        }

        return result;
    }

    ValueOrException operator()(FunctionBody const &v) {
        for (auto const &statement : v.body) {
            auto result = execute(statement);
            if (!result) {
                return result;
            }

            if (returning) {
                auto ret = *std::move(returning);
                returning = std::nullopt;
                return ret;
            }
        }

        return Value{};
    }

    ValueOrException operator()(ReturnStatement const &v) {
        auto ret = v.argument ? execute(*v.argument) : Value{};
        if (!ret) {
            return ret;
        }

        returning = *std::move(ret);
        return Value{};
    }

    ValueOrException operator()(IfStatement const &v) {
        auto test = execute(v.test);
        if (!test) {
            return test;
        }

        if (test->as_bool()) {
            return execute(*v.if_branch);
        }

        return v.else_branch ? execute(**v.else_branch) : Value{};
    }

    ValueOrException operator()(NativeFunction const &v) { return v.f(variables.at("arguments").as_vector()); }

    ValueOrException operator()(EmptyStatement const &) { return Value{}; }

    ValueOrException operator()(WhileStatement const &v) {
        while (true) {
            auto test = execute(v.test);
            if (!test) {
                return test;
            }

            if (!test->as_bool()) {
                return Value{};
            }

            auto result = execute(*v.body);
            if (!result) {
                return result;
            }

            if (returning) {
                return Value{};
            }
        }
    }

    std::map<std::string, Value, std::less<>> variables;
    std::optional<Value> returning;

private:
    // TODO(robinlinden): This should be done in a more generic fashion.
    ValueOrException get_value_resolving_variables(Expression const &expr) {
        if (std::holds_alternative<Identifier>(expr)) {
            auto id = execute(expr);
            assert(id);

            auto var_it = variables.find(id->as_string());
            if (var_it == variables.end()) {
                // TODO(robinlinden): Better error value.
                return tl::unexpected{ErrorValue{Value{}}};
            }

            return var_it->second;
        }

        return execute(expr);
    }
};

} // namespace js::ast

#endif
