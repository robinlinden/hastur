// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "css/media_query.h"

#include "etest/etest2.h"

#include <optional>

namespace {
void parser_tests(etest::Suite &s) {
    s.add_test("parser: missing parens", [](etest::IActions &a) {
        a.expect_eq(css::MediaQuery::parse("width: 300px"), std::nullopt); //
    });

    s.add_test("parser: only feature-name", [](etest::IActions &a) {
        a.expect_eq(css::MediaQuery::parse("(name)"), std::nullopt); //
    });

    s.add_test("parser: missing value", [](etest::IActions &a) {
        a.expect_eq(css::MediaQuery::parse("(name:)"), std::nullopt); //
    });

    s.add_test("parser: invalid value", [](etest::IActions &a) {
        a.expect_eq(css::MediaQuery::parse("(name: abc)"), std::nullopt); //
    });

    s.add_test("parser: unhandled value unit", [](etest::IActions &a) {
        a.expect_eq(css::MediaQuery::parse("(name: 10abc)"), std::nullopt); //
    });

    s.add_test("parser: value with no unit", [](etest::IActions &a) {
        a.expect_eq(css::MediaQuery::parse("(name: 10)"), std::nullopt); //
    });

    s.add_test("parser: 0 is fine w/o a unit", [](etest::IActions &a) {
        a.expect_eq(css::MediaQuery::parse("(max-width: 0)"), css::MediaQuery{css::MediaQuery::Width{.max = 0}}); //
    });

    s.add_test("parser: unhandled feature name", [](etest::IActions &a) {
        a.expect_eq(css::MediaQuery::parse("(disp: 0)"), std::nullopt); //
    });
}

void to_string_tests(etest::Suite &s) {
    s.add_test("to_string", [](etest::IActions &a) {
        a.expect_eq(css::to_string(css::MediaQuery::parse("(width: 300px)").value()), //
                "300 <= width <= 300");
    });

    s.add_test("to_string: width", [](etest::IActions &a) {
        a.expect_eq(css::to_string(css::MediaQuery::Width{.min = 299, .max = 301}), //
                "299 <= width <= 301");
    });

    s.add_test("to_string: false", [](etest::IActions &a) {
        a.expect_eq(css::to_string(css::MediaQuery::False{}), "false"); //
    });
}

void false_tests(etest::Suite &s) {
    s.add_test("false", [](etest::IActions &a) {
        a.expect(!css::MediaQuery::False{}.evaluate({.window_width = 299})); //
    });
}

void width_tests(etest::Suite &s) {
    s.add_test("width: width", [](etest::IActions &a) {
        a.expect_eq(css::MediaQuery::parse("(width: 300px)"),
                css::MediaQuery{css::MediaQuery::Width{.min = 300, .max = 300}});

        auto query = css::MediaQuery::Width{.min = 300, .max = 300};
        a.expect(!query.evaluate({.window_width = 299}));
        a.expect(query.evaluate({.window_width = 300}));
        a.expect(!query.evaluate({.window_width = 301}));
    });

    s.add_test("width: min-width", [](etest::IActions &a) {
        a.expect_eq(css::MediaQuery::parse("(min-width: 300px)"), css::MediaQuery{css::MediaQuery::Width{.min = 300}});

        auto query = css::MediaQuery::Width{.min = 300};
        a.expect(!query.evaluate({.window_width = 299}));
        a.expect(query.evaluate({.window_width = 300}));
        a.expect(query.evaluate({.window_width = 301}));
    });

    s.add_test("width: max-width", [](etest::IActions &a) {
        a.expect_eq(css::MediaQuery::parse("(max-width: 300px)"), css::MediaQuery{css::MediaQuery::Width{.max = 300}});

        auto query = css::MediaQuery::Width{.max = 300};
        a.expect(query.evaluate({.window_width = 299}));
        a.expect(query.evaluate({.window_width = 300}));
        a.expect(!query.evaluate({.window_width = 301}));
    });
}
} // namespace

int main() {
    etest::Suite s{"css::MediaQuery"};
    parser_tests(s);
    to_string_tests(s);
    false_tests(s);
    width_tests(s);
    return s.run();
}
