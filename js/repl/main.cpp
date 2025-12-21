// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "js/ast.h"
#include "js/interpreter.h"
#include "js/parser.h"

#include <iostream>
#include <string>

namespace {

// TODO(robinlinden): Nicer stringification.
std::string to_string(js::ast::Value const &value) {
    if (value.is_undefined()) {
        return "undefined";
    }

    if (value.is_number()) {
        return std::to_string(value.as_number());
    }

    if (value.is_string()) {
        return value.as_string();
    }

    if (value.is_function()) {
        return "[Function]";
    }

    if (value.is_vector()) {
        return "[Array]";
    }

    if (value.is_object()) {
        return "[Object]";
    }

    if (value.is_native_function()) {
        return "[NativeFunction]";
    }

    return "<unhandled>";
}

} // namespace

// TODO(robinlinden): Make the repl nicer:
// * Handle multi-line input.
// * Better error reporting.
// * Command history.
int main() {
    js::ast::Interpreter interpreter;
    std::string input;

    std::cout << "'/quit' to quit.\n";

    while (true) {
        std::cout << "> ";
        std::getline(std::cin, input);

        if (input == "/quit") {
            break;
        }

        if (input.empty()) {
            continue;
        }

        auto ast = js::Parser::parse(input);
        if (!ast) {
            std::cout << "Parse error.\n";
            continue;
        }

        auto result = interpreter.execute(*ast);
        if (!result) {
            std::cout << "Error during execution.\n";
            continue;
        }

        std::cout << to_string(*result) << '\n';
    }
}
