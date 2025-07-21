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
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace js {

class Parser {
public:
    // TODO(robinlinden): Support more than super trivial scripts. This can
    // literally only parse a program w/ a single function call right now.
    static std::optional<ast::Program> parse(std::string_view input) {
        auto maybe_tokens = parse::tokenize(input);
        if (!maybe_tokens) {
            return std::nullopt;
        }

        auto &tokens = *maybe_tokens;
        assert(std::holds_alternative<parse::Eof>(tokens.back()));
        tokens.pop_back();
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
        for (auto it = tokens.begin() + 2; it != tokens.end(); ++it) {
            if (std::holds_alternative<parse::RParen>(*it)) {
                // We reached the end of the arguments.
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

        return ast::Program{
                .body{
                        ast::ExpressionStatement{
                                .expression =
                                        ast::CallExpression{
                                                .callee = std::make_shared<ast::Expression>(
                                                        ast::Identifier{.name = std::move(fn_name.name)}),
                                                .arguments{std::move(args)},
                                        },
                        },
                },
        };
    }
};

} // namespace js

#endif
