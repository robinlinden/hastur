// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "html/parse.h"

#include "dom/dom.h"
#include "etest/etest2.h"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <istream>
#include <optional>
#include <string>
#include <vector>

namespace {

// The blank-line-separated test cases in html5lib-tests/tree-construction/ are
// formatted as follows:
// ```
// #data
// <!DOCTYPE html>
//   Hello
// #errors
// (1, 8): some-made-up-error-ocurred
// (2, 3): another-made-up-error-occurred
// #document
// | <!DOCTYPE html>
// | <html>
// |   <head>
// |   <body>
// |     "Hello"
// ```
// TODO(robinlinden): Test errors as well.
enum class Scripting : std::uint8_t {
    Yes,
    No,
};

struct TestCase {
    std::string input;
    std::string expected_result;
    std::optional<Scripting> scripting{std::nullopt};
};

std::optional<std::vector<TestCase>> parse_test_cases(std::istream &test_bytes) {
    auto tests = std::make_optional<std::vector<TestCase>>();

    std::string line;
    while (test_bytes) {
        auto &test = tests->emplace_back();

        if (!std::getline(test_bytes, line) || line != "#data") {
            std::cerr << "Expected '#data' at the start of a test case.\n";
            return std::nullopt;
        }

        while (std::getline(test_bytes, line) && line != "#errors") {
            test.input += line + '\n';
        }

        while (std::getline(test_bytes, line) && line != "#document" && !line.starts_with("#script")
                && line != "#document-fragment") {
            // Skip the errors for now.
        }

        if (line.starts_with("#script")) {
            if (line == "#script-on") {
                test.scripting = Scripting::Yes;
            } else if (line == "#script-off") {
                test.scripting = Scripting::No;
            } else {
                std::cerr << "Unknown scripting directive: " << line << '\n';
                return std::nullopt;
            }

            if (!std::getline(test_bytes, line) || line != "#document") {
                std::cerr << "Expected '#document' after scripting directive.\n";
                return std::nullopt;
            }
        }

        if (line == "#document-fragment") {
            std::cerr << "Document fragment tests aren't supported yet.\n";
            return std::nullopt;
        }

        if (line != "#document") {
            std::cerr << "Expected '#document' after '#errors'.\n";
            return std::nullopt;
        }

        test.expected_result = "#document\n";
        while (std::getline(test_bytes, line) && !line.empty()) {
            test.expected_result += line + '\n';
        }

        // Remove the trailing newlines.
        test.input.pop_back();
        test.expected_result.pop_back();
    }

    return tests;
}

} // namespace

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "No test file provided\n";
        return 1;
    }

    std::ifstream test_file{argv[1], std::fstream::in | std::fstream::binary};
    if (!test_file) {
        std::cerr << "Failed to open test file '" << argv[1] << "'\n";
        return 1;
    }

    auto tests = parse_test_cases(test_file);
    if (!tests.has_value()) {
        std::cerr << "Error parsing test file.\n";
        return 1;
    }

    etest::Suite s;

    for (auto const &test : *tests) {
        // TODO(robinlinden): Look into how we should treat missing doctype vs empty doctype.
        if (test.input == "<!DOCTYPE >Hello" || test.input == "<!DOCTYPE>Hello") {
            continue;
        }

        s.add_test(test.input, [=](etest::IActions &a) {
            if (test.scripting.value_or(Scripting::No) == Scripting::No) {
                auto document = html::parse(test.input, {.scripting = false, .include_comments = true});
                a.expect_eq(to_string(document), test.expected_result);
            }

            if (test.scripting.value_or(Scripting::Yes) == Scripting::Yes) {
                auto document = html::parse(test.input, {.scripting = true, .include_comments = true});
                a.expect_eq(to_string(document), test.expected_result);
            }
        });
    }

    return s.run();
}
