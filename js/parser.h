// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
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
            auto expr = parse_expression(tokens);
            if (!expr) {
                return std::nullopt;
            }

            program_body.emplace_back(ast::ExpressionStatement{.expression = std::move(*expr)});

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
    [[nodiscard]] static std::optional<ast::Expression> parse_expression(std::span<parse::Token> &tokens) {
        if (tokens.empty()) {
            return std::nullopt;
        }

        if (starts_call_expr(tokens)) {
            return parse_call_expr(tokens);
        }

        if (starts_assign_expr(tokens)) {
            return parse_assign_expr(tokens);
        }

        auto &token = tokens.front();
        if (std::holds_alternative<parse::IntLiteral>(token)) {
            tokens = tokens.subspan(1);
            return ast::NumericLiteral{static_cast<double>(std::get<parse::IntLiteral>(token).value)};
        }

        if (std::holds_alternative<parse::StringLiteral>(token)) {
            tokens = tokens.subspan(1);
            return ast::StringLiteral{std::move(std::get<parse::StringLiteral>(token).value)};
        }

        if (std::holds_alternative<parse::Identifier>(token)) {
            tokens = tokens.subspan(1);
            return ast::Identifier{std::move(std::get<parse::Identifier>(token).name)};
        }

        return std::nullopt;
    }

    [[nodiscard]] static bool starts_call_expr(std::span<parse::Token const> tokens) {
        // Must be at least 3 tokens: identifier, '(', [args [,]] ')'
        return tokens.size() >= 3 && std::holds_alternative<parse::Identifier>(tokens[0])
                && std::holds_alternative<parse::LParen>(tokens[1]);
    }

    // NOLINTNEXTLINE(misc-no-recursion)
    [[nodiscard]] static std::optional<ast::CallExpression> parse_call_expr(std::span<parse::Token> &tokens) {
        assert(starts_call_expr(tokens));

        auto &fn_name = std::get<parse::Identifier>(tokens[0]);
        tokens = tokens.subspan(2); // identifier '('
        std::vector<ast::Expression> args;

        if (std::holds_alternative<parse::RParen>(tokens.front())) {
            tokens = tokens.subspan(1); // ')'
            return ast::CallExpression{
                    .callee = std::make_shared<ast::Expression>(ast::Identifier{.name = std::move(fn_name.name)}),
            };
        }

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
                .callee = std::make_shared<ast::Expression>(ast::Identifier{.name = std::move(fn_name.name)}),
                .arguments{std::move(args)},
        };
    }

    [[nodiscard]] static bool starts_assign_expr(std::span<parse::Token const> tokens) {
        // Must be at least 3 tokens: identifier, '=', value
        return tokens.size() >= 3 && std::holds_alternative<parse::Identifier>(tokens[0])
                && std::holds_alternative<parse::Equals>(tokens[1]);
    }

    [[nodiscard]] static std::optional<ast::AssignmentExpression> parse_assign_expr(std::span<parse::Token> &tokens) {
        assert(starts_assign_expr(tokens));

        auto &var_name = std::get<parse::Identifier>(tokens[0]);
        auto &value_token = tokens[2];

        std::shared_ptr<ast::Expression> value_expr;
        if (std::holds_alternative<parse::IntLiteral>(value_token)) {
            value_expr = std::make_shared<ast::Expression>(
                    ast::NumericLiteral{static_cast<double>(std::get<parse::IntLiteral>(value_token).value)});
        } else if (std::holds_alternative<parse::StringLiteral>(value_token)) {
            value_expr = std::make_shared<ast::Expression>(
                    ast::StringLiteral{std::move(std::get<parse::StringLiteral>(value_token).value)});
        } else if (std::holds_alternative<parse::Identifier>(value_token)) {
            value_expr = std::make_shared<ast::Expression>(
                    ast::Identifier{std::move(std::get<parse::Identifier>(value_token).name)});
        } else {
            return std::nullopt;
        }

        tokens = tokens.subspan(3);

        return ast::AssignmentExpression{
                .left = std::make_shared<ast::Expression>(ast::Identifier{.name = std::move(var_name.name)}),
                .right = std::move(value_expr),
        };
    }
};

} // namespace js

#endif
