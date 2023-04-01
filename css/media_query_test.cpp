// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "css/media_query.h"

#include "etest/etest.h"

using etest::expect;
using etest::expect_eq;

namespace {
void parser_tests() {
    // The comments are to encourage clang-format to format these in a nice way.
    etest::test("parser: missing parens", [] {
        expect_eq(css::MediaQuery::parse("width: 300px"), std::nullopt); //
    });

    etest::test("parser: only feature-name", [] {
        expect_eq(css::MediaQuery::parse("(name)"), std::nullopt); //
    });

    etest::test("parser: missing value", [] {
        expect_eq(css::MediaQuery::parse("(name:)"), std::nullopt); //
    });

    etest::test("parser: invalid value", [] {
        expect_eq(css::MediaQuery::parse("(name: abc)"), std::nullopt); //
    });

    etest::test("parser: unhandled value unit", [] {
        expect_eq(css::MediaQuery::parse("(name: 10abc)"), std::nullopt); //
    });

    etest::test("parser: value with no unit", [] {
        expect_eq(css::MediaQuery::parse("(name: 10)"), std::nullopt); //
    });

    etest::test("parser: 0 is fine w/o a unit", [] {
        expect_eq(css::MediaQuery::parse("(max-width: 0)"), css::MediaQuery{css::MediaQuery::Width{.max = 0}}); //
    });

    etest::test("parser: unhandled feature name", [] {
        expect_eq(css::MediaQuery::parse("(disp: 0)"), std::nullopt); //
    });
}

void width_tests() {
    etest::test("width: width", [] {
        expect_eq(css::MediaQuery::parse("(width: 300px)"),
                css::MediaQuery{css::MediaQuery::Width{.min = 300, .max = 300}});

        auto query = css::MediaQuery::Width{.min = 300, .max = 300};
        expect(!query.evaluate({.window_width = 299}));
        expect(query.evaluate({.window_width = 300}));
        expect(!query.evaluate({.window_width = 301}));
    });

    etest::test("width: min-width", [] {
        expect_eq(css::MediaQuery::parse("(min-width: 300px)"), css::MediaQuery{css::MediaQuery::Width{.min = 300}});

        auto query = css::MediaQuery::Width{.min = 300};
        expect(!query.evaluate({.window_width = 299}));
        expect(query.evaluate({.window_width = 300}));
        expect(query.evaluate({.window_width = 301}));
    });

    etest::test("width: max-width", [] {
        expect_eq(css::MediaQuery::parse("(max-width: 300px)"), css::MediaQuery{css::MediaQuery::Width{.max = 300}});

        auto query = css::MediaQuery::Width{.max = 300};
        expect(query.evaluate({.window_width = 299}));
        expect(query.evaluate({.window_width = 300}));
        expect(!query.evaluate({.window_width = 301}));
    });
}
} // namespace

int main() {
    parser_tests();
    width_tests();
    return etest::run_all_tests();
}
