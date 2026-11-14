// SPDX-FileCopyrightText: 2022-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef JS_INTERPRETER_H_
#define JS_INTERPRETER_H_

#include "js/ast.h"

#include <array>
#include <cassert>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <expected>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <system_error>
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

    ValueOrException operator()(Identifier const &v) {
        auto var = variables.find(v.name);
        if (var == variables.end()) {
            // TODO(robinlinden): Better error value.
            return std::unexpected{ErrorValue{Value{}}};
        }

        return var->second;
    }

    ValueOrException operator()(Pattern const &v) { return std::visit(*this, v); }
    ValueOrException operator()(Statement const &v) { return std::visit(*this, v); }

    ValueOrException operator()(ExpressionStatement const &v) { return execute(v.expression); }

    ValueOrException operator()(AssignmentExpression const &v) {
        auto *maybe_id = std::get_if<Identifier>(&*v.left);
        if (maybe_id == nullptr) {
            // TODO(robinlinden): Better error value.
            return std::unexpected{ErrorValue{Value{}}};
        }

        auto value = execute(*v.right);
        if (!value) {
            return value;
        }

        return variables[maybe_id->name] = *std::move(value);
    }

    ValueOrException operator()(BinaryExpression const &v) {
        auto lhs = execute(*v.lhs);
        if (!lhs) {
            return lhs;
        }

        auto rhs = execute(*v.rhs);
        if (!rhs) {
            return rhs;
        }

        if (!lhs->is_number() || !rhs->is_number()) {
            // TODO(robinlinden): Support more types in binary expressions. We
            // want 1 + "0" - 1 to be 9, obviously.
            return std::unexpected{ErrorValue{Value{}}};
        }

        switch (v.op) {
            case BinaryOperator::Plus:
                return Value{lhs->as_number() + rhs->as_number()};
            case BinaryOperator::Minus:
                return Value{lhs->as_number() - rhs->as_number()};
            case BinaryOperator::Times:
                return Value{lhs->as_number() * rhs->as_number()};
            case BinaryOperator::Divide:
                if (rhs->as_number() == 0) {
                    // TODO(robinlinden): Better error value.
                    return std::unexpected{ErrorValue{Value{}}};
                }

                return Value{lhs->as_number() / rhs->as_number()};
        }

        return std::unexpected{ErrorValue{Value{}}};
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
        auto name = get_identifier_name(v.id);
        ValueOrException init_value = v.init ? execute(*v.init) : Value{};
        if (!init_value) {
            return init_value;
        }

        variables[std::move(name)] = *std::move(init_value);
        return Value{};
    }

    ValueOrException operator()(FunctionDeclaration const &v) {
        variables[v.id.name] = Value{v.function};
        return Value{};
    }

    ValueOrException operator()(FunctionExpression const &v) { return Value{v.function}; }

    ValueOrException operator()(ObjectExpression const &v) {
        Object obj;
        for (auto const &prop : v.properties) {
            auto value = execute(prop.second);
            if (!value) {
                return value;
            }

            obj[prop.first.name] = *std::move(value);
        }

        return Value{std::move(obj)};
    }

    ValueOrException operator()(CallExpression const &v) {
        Interpreter scope{*this};

        auto callee = execute(*v.callee);
        if (!callee) {
            return callee;
        }

        if (!callee->is_function() && !callee->is_native_function()) {
            // TODO(robinlinden): Better error value.
            return std::unexpected{ErrorValue{Value{}}};
        }

        std::vector<Value> args;
        args.reserve(v.arguments.size());
        for (auto const &arg : v.arguments) {
            auto arg_value = execute(arg);
            if (!arg_value) {
                return arg_value;
            }

            args.push_back(*std::move(arg_value));
        }

        // https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Functions/arguments
        scope.variables["arguments"] = Value{std::move(args)};
        if (callee->is_function()) {
            return scope.execute(*callee->as_function());
        }

        return scope.execute(callee->as_native_function());
    }

    ValueOrException operator()(MemberExpression const &v) {
        auto object = execute(*v.object);
        if (!object) {
            return object;
        }

        // TODO(robinlinden): "foo".length and similar should be allowed.
        if (!object->is_object()) {
            return std::unexpected{ErrorValue{Value{}}};
        }

        auto property = get_identifier_name(v.property);

        auto const &obj = object->as_object();
        auto it = obj.find(property);
        if (it == obj.end()) {
            // TODO(robinlinden): Better error value.
            return std::unexpected{ErrorValue{Value{}}};
        }

        return it->second;
    }

    ValueOrException operator()(ComputedMemberExpression const &v) {
        auto object = execute(*v.object);
        if (!object) {
            return object;
        }

        auto property = execute(*v.property);
        if (!property) {
            return property;
        }

        return get_computed_member(*object, *property);
    }

    ValueOrException operator()(Function const &v) {
        auto const &args = variables.at("arguments").as_vector();
        for (std::size_t i = 0; i < v.params.size(); ++i) {
            auto id = get_identifier_name(v.params[i]);
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
    static std::string const &get_identifier_name(Pattern const &p) { return std::get<Identifier>(p).name; }

    static ValueOrException get_computed_member(Value const &object, Value const &property) {
        if (object.is_vector() && property.is_number()) {
            auto index = property.as_number();
            auto const &elements = object.as_vector();
            if (!std::isfinite(index) || index < 0 || index >= static_cast<double>(elements.size())
                    || std::trunc(index) != index) {
                return std::unexpected{ErrorValue{Value{}}};
            }

            return elements[static_cast<std::size_t>(index)];
        }

        if (!object.is_object()) {
            return std::unexpected{ErrorValue{Value{}}};
        }

        std::string property_name;
        if (property.is_string()) {
            property_name = property.as_string();
        } else if (property.is_number()) {
            auto index = property.as_number();
            if (!std::isfinite(index)) {
                return std::unexpected{ErrorValue{Value{}}};
            }

            std::array<char, 32> buffer{};
            auto [end, error] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), index);
            // There's no way this could fail, since the buffer is large enough to hold any double.
            assert(error == std::errc{});
            property_name.assign(buffer.data(), end);
        } else {
            return std::unexpected{ErrorValue{Value{}}};
        }

        auto const &obj = object.as_object();
        auto it = obj.find(property_name);
        if (it == obj.end()) {
            return std::unexpected{ErrorValue{Value{}}};
        }

        return it->second;
    }
};

} // namespace js::ast

#endif
