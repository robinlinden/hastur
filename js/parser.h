// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef JS_PARSER_H_
#define JS_PARSER_H_

#include "js/ast.h"
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
            auto call = parse_call_expr(tokens);
            if (!call) {
                return std::nullopt;
            }

            program_body.emplace_back(ast::ExpressionStatement{.expression = std::move(*call)});

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
    static std::optional<ast::CallExpression> parse_call_expr(std::span<parse::Token> &tokens) {
        if (tokens.size() < 3) {
            return std::nullopt;
        }

        // functionname(
        if (!std::holds_alternative<parse::Identifier>(tokens[0])
                || !std::holds_alternative<parse::LParen>(tokens[1])) {
            return std::nullopt;
        }

        constexpr auto kMakeArg = [](parse::Token &token) -> std::optional<std::shared_ptr<ast::Expression>> {
            if (std::holds_alternative<parse::IntLiteral>(token)) {
                return std::make_shared<ast::Expression>(
                        ast::NumericLiteral{static_cast<double>(std::get<parse::IntLiteral>(token).value)});
            }

            if (std::holds_alternative<parse::StringLiteral>(token)) {
                return std::make_shared<ast::Expression>(
                        ast::StringLiteral{std::move(std::get<parse::StringLiteral>(token).value)});
            }

            if (std::holds_alternative<parse::Identifier>(token)) {
                return std::make_shared<ast::Expression>(
                        ast::Identifier{std::move(std::get<parse::Identifier>(token).name)});
            }

            return std::nullopt;
        };

        auto &fn_name = std::get<parse::Identifier>(tokens[0]);
        std::vector<std::shared_ptr<ast::Expression>> args;

        // arg1, arg2, arg3)
        bool found_rparen{false};
        for (auto it = tokens.begin() + 2; it != tokens.end(); ++it) {
            if (std::holds_alternative<parse::RParen>(*it)) {
                // We reached the end of the arguments.
                found_rparen = true;
                break;
            }

            auto const parsing_first_arg = args.empty();
            if (!parsing_first_arg && !std::holds_alternative<parse::Comma>(*it)) {
                // We expected a comma, but didn't find one.
                return std::nullopt;
            }

            if (!parsing_first_arg) {
                // Skip the comma.
                ++it;
                if (it == tokens.end()) {
                    // We reached the end of the input, but expected more arguments.
                    return std::nullopt;
                }
            }

            auto arg = kMakeArg(*it);
            if (!arg) {
                return std::nullopt;
            }

            args.push_back(std::move(*arg));
        }

        if (!found_rparen) {
            return std::nullopt;
        }

        // Each arg has a comma, except the last one.
        auto const arg_token_count = args.empty() ? 0 : args.size() * 2 - 1;
        auto const function_token_count = 3 + arg_token_count; // fn_name + ( + args + )
        assert(tokens.size() >= function_token_count);

        tokens = tokens.subspan(function_token_count);

        return ast::CallExpression{
                .callee = std::make_shared<ast::Expression>(ast::Identifier{.name = std::move(fn_name.name)}),
                .arguments{std::move(args)},
        };
    }
};

} // namespace js

#endif
