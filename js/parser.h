// SPDX-FileCopyrightText: 2025-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef JS_PARSER_H_
#define JS_PARSER_H_

#include "js/ast.h"
#include "js/token.h"
#include "js/tokenizer.h"

#include <cassert>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace js {

class Parser {
public:
    // TODO(robinlinden): Support more than super trivial scripts.
    static std::optional<Program> parse(std::string_view input) {
        auto maybe_tokens = parse::tokenize(input);
        if (!maybe_tokens) {
            return std::nullopt;
        }

        std::span<parse::Token> tokens = *maybe_tokens;
        assert(std::holds_alternative<parse::Eof>(tokens.back()));
        tokens = tokens.subspan(0, tokens.size() - 1); // Remove EOF.

        std::vector<Statement> program_body;

        while (!tokens.empty()) {
            auto stmt = parse_statement(tokens);
            if (!stmt) {
                return std::nullopt;
            }

            program_body.emplace_back(std::move(*stmt));

            // TODO(robinlinden): Automatic semicolon insertion.
            if (needs_semicolon(program_body.back())) {
                if (!next_is_semicolon(tokens)) {
                    return std::nullopt;
                }

                tokens = tokens.subspan(1);
            }
        }

        return Program{.body = std::move(program_body)};
    }

private:
    [[nodiscard]] static bool needs_semicolon(Statement const &stmt) {
        return !std::holds_alternative<EmptyStatement>(stmt) && !std::holds_alternative<FunctionDeclaration>(stmt);
    }

    [[nodiscard]] static bool next_is_semicolon(std::span<parse::Token> tokens) {
        return !tokens.empty() && std::holds_alternative<parse::Semicolon>(tokens.front());
    }

    // NOLINTNEXTLINE(misc-no-recursion)
    [[nodiscard]] static std::optional<Statement> parse_statement(std::span<parse::Token> &tokens) {
        if (std::holds_alternative<parse::Semicolon>(tokens.front())) {
            tokens = tokens.subspan(1); // ';'
            return EmptyStatement{};
        }

        if (std::holds_alternative<parse::Function>(tokens.front())) {
            return parse_function_declaration(tokens);
        }

        if (std::holds_alternative<parse::Return>(tokens.front())) {
            tokens = tokens.subspan(1); // 'return'
            if (tokens.empty()) {
                return std::nullopt;
            }

            if (std::holds_alternative<parse::Semicolon>(tokens.front())) {
                return ReturnStatement{.argument = std::nullopt};
            }

            std::optional<Expression> argument = parse_expression(tokens);
            if (!argument) {
                return std::nullopt;
            }

            return ReturnStatement{.argument = std::move(argument)};
        }

        if (std::holds_alternative<parse::Var>(tokens.front())) {
            tokens = tokens.subspan(1); // 'var'
            if (tokens.empty() || !std::holds_alternative<parse::Identifier>(tokens.front())) {
                return std::nullopt;
            }

            auto &var_name = std::get<parse::Identifier>(tokens.front());
            tokens = tokens.subspan(1); // identifier

            std::optional<Expression> init;
            if (!tokens.empty() && std::holds_alternative<parse::Equals>(tokens.front())) {
                tokens = tokens.subspan(1); // '='
                init = parse_expression(tokens);
                if (!init) {
                    return std::nullopt;
                }
            }

            // TODO(robinlinden): Support multiple variable declarations in a single statement.

            return VariableDeclaration{
                    .declarations{
                            VariableDeclarator{
                                    .id = Identifier{.name = std::move(var_name.name)},
                                    .init = std::move(init),
                            },
                    },
                    .kind = VariableDeclaration::Kind::Var,
            };
        }

        auto expr = parse_expression(tokens);
        if (!expr) {
            return std::nullopt;
        }

        return ExpressionStatement{.expression = std::move(*expr)};
    }

    // Either a FunctionExpression or a FunctionDeclaration. The name is
    // optional for FunctionExpressions.
    struct ParsedFunction {
        std::optional<Identifier> name;
        std::shared_ptr<Function> function;
    };

    // NOLINTNEXTLINE(misc-no-recursion)
    [[nodiscard]] static std::optional<ParsedFunction> parse_function(std::span<parse::Token> &tokens) {
        assert(std::holds_alternative<parse::Function>(tokens.front()));
        tokens = tokens.subspan(1); // 'function'

        if (tokens.empty()) {
            return std::nullopt;
        }

        std::optional<Identifier> fn_name;
        if (std::holds_alternative<parse::Identifier>(tokens.front())) {
            fn_name = Identifier{
                    .name = std::move(std::get<parse::Identifier>(tokens.front()).name),
            };

            tokens = tokens.subspan(1); // identifier
        }

        if (tokens.empty() || !std::holds_alternative<parse::LParen>(tokens.front())) {
            return std::nullopt;
        }

        tokens = tokens.subspan(1); // '('
        if (tokens.empty()) {
            return std::nullopt;
        }

        std::vector<Pattern> params;
        while (std::holds_alternative<parse::Identifier>(tokens.front())) {
            auto *param_name = std::get_if<parse::Identifier>(&tokens.front());
            assert(param_name != nullptr);

            params.emplace_back(Identifier{.name = std::move(param_name->name)});
            tokens = tokens.subspan(1); // identifier
            if (tokens.empty()) {
                return std::nullopt;
            }

            if (std::holds_alternative<parse::RParen>(tokens.front())) {
                break;
            }

            if (!std::holds_alternative<parse::Comma>(tokens.front())) {
                return std::nullopt;
            }

            tokens = tokens.subspan(1); // ','
            if (tokens.empty()) {
                return std::nullopt;
            }
        }

        if (!std::holds_alternative<parse::RParen>(tokens.front())) {
            return std::nullopt;
        }

        tokens = tokens.subspan(1); // ')'

        if (tokens.empty() || !std::holds_alternative<parse::LBrace>(tokens.front())) {
            return std::nullopt;
        }

        tokens = tokens.subspan(1); // '{'
        if (tokens.empty()) {
            return std::nullopt;
        }

        FunctionBody body;
        while (!std::holds_alternative<parse::RBrace>(tokens.front())) {
            auto stmt = parse_statement(tokens);
            if (!stmt) {
                return std::nullopt;
            }

            body.body.push_back(std::move(*stmt));

            if (tokens.empty()) {
                return std::nullopt;
            }

            if (std::holds_alternative<parse::Semicolon>(tokens.front())) {
                tokens = tokens.subspan(1); // ';'
                if (tokens.empty()) {
                    return std::nullopt;
                }
            } else if (!std::holds_alternative<parse::RBrace>(tokens.front())) {
                return std::nullopt;
            }
        }

        assert(std::holds_alternative<parse::RBrace>(tokens.front()));
        tokens = tokens.subspan(1); // '}'

        auto function = std::make_shared<js::Function>(js::Function{
                .params{std::move(params)},
                .body{std::move(body)},
        });

        return ParsedFunction{
                .name = std::move(fn_name),
                .function = std::move(function),
        };
    }

    // NOLINTNEXTLINE(misc-no-recursion)
    [[nodiscard]] static std::optional<FunctionDeclaration> parse_function_declaration(
            std::span<parse::Token> &tokens) {
        auto maybe_fn = parse_function(tokens);
        if (!maybe_fn || !maybe_fn->name.has_value()) {
            return std::nullopt;
        }

        auto &[fn_name, function] = *maybe_fn;
        return FunctionDeclaration{
                .id = std::move(*fn_name),
                .function = std::move(function),
        };
    }

    // NOLINTNEXTLINE(misc-no-recursion)
    [[nodiscard]] static std::optional<FunctionExpression> parse_function_expression(std::span<parse::Token> &tokens) {
        auto maybe_fn = parse_function(tokens);
        if (!maybe_fn) {
            return std::nullopt;
        }

        auto &[fn_name, function] = *maybe_fn;
        return FunctionExpression{
                .id = std::move(fn_name),
                .function = std::move(function),
        };
    }

    // NOLINTNEXTLINE(misc-no-recursion)
    [[nodiscard]] static std::optional<ObjectExpression> parse_object_expression(std::span<parse::Token> &tokens) {
        if (!std::holds_alternative<parse::LBrace>(tokens.front())) {
            return std::nullopt;
        }

        tokens = tokens.subspan(1); // '{'

        if (tokens.empty()) {
            return std::nullopt;
        }

        std::vector<std::pair<Identifier, Expression>> properties;

        while (!std::holds_alternative<parse::RBrace>(tokens.front())) {
            auto *key = std::get_if<parse::Identifier>(&tokens.front());
            if (key == nullptr) {
                return std::nullopt;
            }

            tokens = tokens.subspan(1); // identifier

            if (tokens.empty() || !std::holds_alternative<parse::Colon>(tokens.front())) {
                return std::nullopt;
            }

            tokens = tokens.subspan(1); // ':'

            auto value = parse_expression(tokens);
            if (!value) {
                return std::nullopt;
            }

            properties.emplace_back(Identifier{.name = std::move(key->name)}, std::move(*value));

            if (tokens.empty()) {
                return std::nullopt;
            }

            if (std::holds_alternative<parse::Comma>(tokens.front())) {
                tokens = tokens.subspan(1); // ','

                if (tokens.empty()) {
                    return std::nullopt;
                }
            }
        }

        tokens = tokens.subspan(1); // '}'

        return ObjectExpression{.properties{std::move(properties)}};
    }

    // NOLINTNEXTLINE(misc-no-recursion)
    [[nodiscard]] static std::optional<Expression> parse_expression(std::span<parse::Token> &tokens) {
        auto expr = parse_primary_expression(tokens);
        if (!expr) {
            return std::nullopt;
        }

        if (!tokens.empty() && binary_operator(tokens.front())) {
            return parse_binary_expression(tokens, std::move(*expr), 0);
        }

        if (!tokens.empty() && std::holds_alternative<parse::Equals>(tokens.front())) {
            tokens = tokens.subspan(1); // '='
            return parse_assign_expr(std::make_shared<Expression>(std::move(*expr)), tokens);
        }

        return expr;
    }

    [[nodiscard]] static std::optional<std::pair<BinaryOperator, int>> binary_operator(parse::Token const &token) {
        if (std::holds_alternative<parse::Plus>(token)) {
            return std::pair{BinaryOperator::Plus, 1};
        }

        if (std::holds_alternative<parse::Minus>(token)) {
            return std::pair{BinaryOperator::Minus, 1};
        }

        if (std::holds_alternative<parse::Asterisk>(token)) {
            return std::pair{BinaryOperator::Times, 2};
        }

        if (std::holds_alternative<parse::Slash>(token)) {
            return std::pair{BinaryOperator::Divide, 2};
        }

        return std::nullopt;
    }

    // NOLINTNEXTLINE(misc-no-recursion)
    [[nodiscard]] static std::optional<Expression> parse_binary_expression(
            std::span<parse::Token> &tokens, Expression expr, int min_precedence) {
        while (!tokens.empty()) {
            auto op = binary_operator(tokens.front());
            if (!op || op->second < min_precedence) {
                break;
            }

            tokens = tokens.subspan(1);
            auto rhs = parse_primary_expression(tokens);
            if (!rhs) {
                return std::nullopt;
            }

            if (!tokens.empty()) {
                auto next_op = binary_operator(tokens.front());
                if (next_op && next_op->second > op->second) {
                    rhs = parse_binary_expression(tokens, std::move(*rhs), op->second + 1);
                    if (!rhs) {
                        return std::nullopt;
                    }
                }
            }

            expr = BinaryExpression{
                    .op = op->first,
                    .lhs = std::make_shared<Expression>(std::move(expr)),
                    .rhs = std::make_shared<Expression>(std::move(*rhs)),
            };
        }

        return expr;
    }

    // NOLINTNEXTLINE(misc-no-recursion)
    [[nodiscard]] static std::optional<Expression> parse_primary_expression(std::span<parse::Token> &tokens) {
        if (tokens.empty()) {
            return std::nullopt;
        }

        std::optional<Expression> expr;

        if (auto &token = tokens.front(); std::holds_alternative<parse::IntLiteral>(token)) {
            tokens = tokens.subspan(1);
            expr = NumericLiteral{static_cast<double>(std::get<parse::IntLiteral>(token).value)};
        } else if (std::holds_alternative<parse::StringLiteral>(token)) {
            tokens = tokens.subspan(1);
            expr = StringLiteral{std::move(std::get<parse::StringLiteral>(token).value)};
        } else if (std::holds_alternative<parse::Identifier>(token)) {
            tokens = tokens.subspan(1);
            expr = Identifier{std::move(std::get<parse::Identifier>(token).name)};
        } else if (std::holds_alternative<parse::Function>(tokens.front())) {
            expr = parse_function_expression(tokens);
        } else if (std::holds_alternative<parse::LBrace>(tokens.front())) {
            expr = parse_object_expression(tokens);
        } else {
            return std::nullopt;
        }

        while (!tokens.empty() && expr.has_value()) {
            if (std::holds_alternative<parse::Period>(tokens.front())) {
                tokens = tokens.subspan(1); // '.'
                expr = parse_member_expr(std::make_shared<Expression>(*std::move(expr)), tokens);
            } else if (std::holds_alternative<parse::LBracket>(tokens.front())) {
                tokens = tokens.subspan(1); // '['
                auto property = parse_expression(tokens);
                if (!property || tokens.empty() || !std::holds_alternative<parse::RBracket>(tokens.front())) {
                    return std::nullopt;
                }

                tokens = tokens.subspan(1); // ']'
                expr = ComputedMemberExpression{
                        .object = std::make_shared<Expression>(std::move(*expr)),
                        .property = std::make_shared<Expression>(std::move(*property)),
                };
            } else if (std::holds_alternative<parse::LParen>(tokens.front())) {
                tokens = tokens.subspan(1); // '('
                expr = parse_call_expr(std::make_shared<Expression>(*std::move(expr)), tokens);
            } else {
                return expr;
            }
        }

        return expr;
    }

    // NOLINTNEXTLINE(misc-no-recursion)
    [[nodiscard]] static std::optional<CallExpression> parse_call_expr(
            std::shared_ptr<Expression> callee, std::span<parse::Token> &tokens) {
        if (tokens.empty()) {
            return std::nullopt;
        }

        if (std::holds_alternative<parse::RParen>(tokens.front())) {
            tokens = tokens.subspan(1); // ')'
            return CallExpression{.callee = std::move(callee)};
        }

        std::vector<Expression> args;
        // arg1, arg2, arg3)
        while (true) {
            auto arg = parse_expression(tokens);
            if (!arg) {
                return std::nullopt;
            }

            args.push_back(std::move(*arg));

            if (tokens.empty()) {
                return std::nullopt;
            }

            if (std::holds_alternative<parse::RParen>(tokens.front())) {
                tokens = tokens.subspan(1);
                break;
            }

            if (!std::holds_alternative<parse::Comma>(tokens.front())) {
                return std::nullopt;
            }

            tokens = tokens.subspan(1); // ','
        }

        return CallExpression{
                .callee = std::move(callee),
                .arguments{std::move(args)},
        };
    }

    // NOLINTNEXTLINE(misc-no-recursion)
    [[nodiscard]] static std::optional<AssignmentExpression> parse_assign_expr(
            std::shared_ptr<Expression> lhs, std::span<parse::Token> &tokens) {
        auto value_expr = parse_expression(tokens);
        if (!value_expr) {
            return std::nullopt;
        }

        return AssignmentExpression{
                .left = std::move(lhs),
                .right = std::make_shared<Expression>(std::move(*value_expr)),
        };
    }

    [[nodiscard]] static std::optional<MemberExpression> parse_member_expr(
            std::shared_ptr<Expression> object, std::span<parse::Token> &tokens) {
        if (tokens.empty() || !std::holds_alternative<parse::Identifier>(tokens.front())) {
            return std::nullopt;
        }

        auto &property_name = std::get<parse::Identifier>(tokens.front());
        tokens = tokens.subspan(1);

        return MemberExpression{
                .object = std::move(object),
                .property = Identifier{.name = std::move(property_name.name)},
        };
    }
};

} // namespace js

#endif
