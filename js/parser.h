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
    static std::optional<ast::Program> parse(std::string_view input) {
        auto maybe_tokens = parse::tokenize(input);
        if (!maybe_tokens) {
            return std::nullopt;
        }

        std::span<parse::Token> tokens = *maybe_tokens;
        assert(std::holds_alternative<parse::Eof>(tokens.back()));
        tokens = tokens.subspan(0, tokens.size() - 1); // Remove EOF.

        std::vector<ast::Statement> program_body;

        while (!tokens.empty()) {
            auto stmt = parse_statement(tokens);
            if (!stmt) {
                return std::nullopt;
            }

            program_body.emplace_back(std::move(*stmt));

            if (!tokens.empty()) {
                if (!std::holds_alternative<parse::Semicolon>(tokens.front())) {
                    return std::nullopt;
                }

                tokens = tokens.subspan(1);
            }
        }

        return ast::Program{.body = std::move(program_body)};
    }

private:
    // NOLINTNEXTLINE(misc-no-recursion)
    [[nodiscard]] static std::optional<ast::Statement> parse_statement(std::span<parse::Token> &tokens) {
        if (std::holds_alternative<parse::Function>(tokens.front())) {
            return parse_function_declaration(tokens);
        }

        if (std::holds_alternative<parse::Return>(tokens.front())) {
            tokens = tokens.subspan(1); // 'return'
            if (tokens.empty()) {
                return std::nullopt;
            }

            if (std::holds_alternative<parse::Semicolon>(tokens.front())) {
                return ast::ReturnStatement{.argument = std::nullopt};
            }

            std::optional<ast::Expression> argument = parse_expression(tokens);
            if (!argument) {
                return std::nullopt;
            }

            return ast::ReturnStatement{.argument = std::move(argument)};
        }

        auto expr = parse_expression(tokens);
        if (!expr) {
            return std::nullopt;
        }

        return ast::ExpressionStatement{.expression = std::move(*expr)};
    }

    // Either a FunctionExpression or a FunctionDeclaration. The name is
    // optional for FunctionExpressions.
    struct Function {
        std::optional<ast::Identifier> name;
        std::shared_ptr<ast::Function> function;
    };

    // NOLINTNEXTLINE(misc-no-recursion)
    [[nodiscard]] static std::optional<Function> parse_function(std::span<parse::Token> &tokens) {
        assert(std::holds_alternative<parse::Function>(tokens.front()));
        tokens = tokens.subspan(1); // 'function'

        if (tokens.empty()) {
            return std::nullopt;
        }

        std::optional<ast::Identifier> fn_name;
        if (std::holds_alternative<parse::Identifier>(tokens.front())) {
            fn_name = ast::Identifier{
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

        std::vector<ast::Pattern> params;
        while (std::holds_alternative<parse::Identifier>(tokens.front())) {
            auto *param_name = std::get_if<parse::Identifier>(&tokens.front());
            assert(param_name != nullptr);

            params.emplace_back(ast::Identifier{.name = std::move(param_name->name)});
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

        ast::FunctionBody body;
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

        auto function = std::make_shared<ast::Function>(ast::Function{
                .params{std::move(params)},
                .body{std::move(body)},
        });

        return Function{
                .name = std::move(fn_name),
                .function = std::move(function),
        };
    }

    // NOLINTNEXTLINE(misc-no-recursion)
    [[nodiscard]] static std::optional<ast::FunctionDeclaration> parse_function_declaration(
            std::span<parse::Token> &tokens) {
        auto maybe_fn = parse_function(tokens);
        if (!maybe_fn || !maybe_fn->name.has_value()) {
            return std::nullopt;
        }

        auto &[fn_name, function] = *maybe_fn;
        return ast::FunctionDeclaration{
                .id = std::move(*fn_name),
                .function = std::move(function),
        };
    }

    // NOLINTNEXTLINE(misc-no-recursion)
    [[nodiscard]] static std::optional<ast::Expression> parse_expression(std::span<parse::Token> &tokens) {
        if (tokens.empty()) {
            return std::nullopt;
        }

        std::optional<ast::Expression> expr;

        if (auto &token = tokens.front(); std::holds_alternative<parse::IntLiteral>(token)) {
            tokens = tokens.subspan(1);
            expr = ast::NumericLiteral{static_cast<double>(std::get<parse::IntLiteral>(token).value)};
        } else if (std::holds_alternative<parse::StringLiteral>(token)) {
            tokens = tokens.subspan(1);
            expr = ast::StringLiteral{std::move(std::get<parse::StringLiteral>(token).value)};
        } else if (std::holds_alternative<parse::Identifier>(token)) {
            tokens = tokens.subspan(1);
            expr = ast::Identifier{std::move(std::get<parse::Identifier>(token).name)};
        } else {
            return std::nullopt;
        }

        while (!tokens.empty() && expr.has_value()) {
            if (std::holds_alternative<parse::Period>(tokens.front())) {
                tokens = tokens.subspan(1); // '.'
                expr = parse_member_expr(std::make_shared<ast::Expression>(*std::move(expr)), tokens);
            } else if (std::holds_alternative<parse::LParen>(tokens.front())) {
                tokens = tokens.subspan(1); // '('
                expr = parse_call_expr(std::make_shared<ast::Expression>(*std::move(expr)), tokens);
            } else if (std::holds_alternative<parse::Equals>(tokens.front())) {
                tokens = tokens.subspan(1); // '='
                expr = parse_assign_expr(std::make_shared<ast::Expression>(*std::move(expr)), tokens);
            } else {
                return expr;
            }
        }

        return expr;
    }

    // NOLINTNEXTLINE(misc-no-recursion)
    [[nodiscard]] static std::optional<ast::CallExpression> parse_call_expr(
            std::shared_ptr<ast::Expression> callee, std::span<parse::Token> &tokens) {
        if (tokens.empty()) {
            return std::nullopt;
        }

        if (std::holds_alternative<parse::RParen>(tokens.front())) {
            tokens = tokens.subspan(1); // ')'
            return ast::CallExpression{.callee = std::move(callee)};
        }

        std::vector<ast::Expression> args;
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

        return ast::CallExpression{
                .callee = std::move(callee),
                .arguments{std::move(args)},
        };
    }

    // NOLINTNEXTLINE(misc-no-recursion)
    [[nodiscard]] static std::optional<ast::AssignmentExpression> parse_assign_expr(
            std::shared_ptr<ast::Expression> lhs, std::span<parse::Token> &tokens) {
        auto value_expr = parse_expression(tokens);
        if (!value_expr) {
            return std::nullopt;
        }

        return ast::AssignmentExpression{
                .left = std::move(lhs),
                .right = std::make_shared<ast::Expression>(std::move(*value_expr)),
        };
    }

    [[nodiscard]] static std::optional<ast::MemberExpression> parse_member_expr(
            std::shared_ptr<ast::Expression> object, std::span<parse::Token> &tokens) {
        if (tokens.empty() || !std::holds_alternative<parse::Identifier>(tokens.front())) {
            return std::nullopt;
        }

        auto &property_name = std::get<parse::Identifier>(tokens.front());
        tokens = tokens.subspan(1);

        return ast::MemberExpression{
                .object = std::move(object),
                .property = ast::Identifier{.name = std::move(property_name.name)},
        };
    }
};

} // namespace js

#endif
