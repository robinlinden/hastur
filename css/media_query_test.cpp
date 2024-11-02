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

    s.add_test("to_string: prefers-color-scheme", [](etest::IActions &a) {
        a.expect_eq(css::to_string(css::MediaQuery::PrefersColorScheme{.color_scheme = css::ColorScheme::Light}), //
                "prefers-color-scheme: light");
    });

    s.add_test("to_string: type", [](etest::IActions &a) {
        a.expect_eq(css::to_string(css::MediaQuery::Type{.type = css::MediaType::Print}), "print");
        a.expect_eq(css::to_string(css::MediaQuery::Type{.type = css::MediaType::Screen}), "screen");
    });

    s.add_test("to_string: width", [](etest::IActions &a) {
        a.expect_eq(css::to_string(css::MediaQuery::Width{.min = 299, .max = 301}), //
                "299 <= width <= 301");
    });

    s.add_test("to_string: false", [](etest::IActions &a) {
        a.expect_eq(css::to_string(css::MediaQuery::False{}), "false"); //
    });

    s.add_test("to_string: true", [](etest::IActions &a) {
        a.expect_eq(css::to_string(css::MediaQuery::True{}), "true"); //
    });
}

void false_tests(etest::Suite &s) {
    s.add_test("false", [](etest::IActions &a) {
        a.expect(!css::MediaQuery::False{}.evaluate({.window_width = 299})); //
    });
}

void true_tests(etest::Suite &s) {
    s.add_test("true", [](etest::IActions &a) {
        a.expect(css::MediaQuery::True{}.evaluate({})); //
    });
}

void prefers_color_scheme_tests(etest::Suite &s) {
    s.add_test("prefers-color-scheme: light", [](etest::IActions &a) {
        a.expect_eq(css::MediaQuery::parse("(prefers-color-scheme: light)"),
                css::MediaQuery{css::MediaQuery::PrefersColorScheme{.color_scheme = css::ColorScheme::Light}});

        auto query = css::MediaQuery::PrefersColorScheme{.color_scheme = css::ColorScheme::Light};
        a.expect(query.evaluate({.color_scheme = css::ColorScheme::Light}));
        a.expect(!query.evaluate({.color_scheme = css::ColorScheme::Dark}));
    });

    s.add_test("prefers-color-scheme: dark", [](etest::IActions &a) {
        a.expect_eq(css::MediaQuery::parse("(prefers-color-scheme: dark)"),
                css::MediaQuery{css::MediaQuery::PrefersColorScheme{.color_scheme = css::ColorScheme::Dark}});

        auto query = css::MediaQuery::PrefersColorScheme{.color_scheme = css::ColorScheme::Dark};
        a.expect(!query.evaluate({.color_scheme = css::ColorScheme::Light}));
        a.expect(query.evaluate({.color_scheme = css::ColorScheme::Dark}));
    });

    s.add_test("prefers-color-scheme: invalid-value", [](etest::IActions &a) {
        a.expect_eq(css::MediaQuery::parse("(prefers-color-scheme: invalid)"), std::nullopt); //
    });
}

void type_tests(etest::Suite &s) {
    s.add_test("type", [](etest::IActions &a) {
        a.expect_eq(css::MediaQuery::parse("all"), css::MediaQuery{css::MediaQuery::True{}});

        a.expect_eq(css::MediaQuery::parse("print"),
                css::MediaQuery{css::MediaQuery::Type{.type = css::MediaType::Print}} //
        );
        a.expect_eq(css::MediaQuery::parse("screen"),
                css::MediaQuery{css::MediaQuery::Type{.type = css::MediaType::Screen}} //
        );
        a.expect_eq(css::MediaQuery::parse("only all"), css::MediaQuery{css::MediaQuery::True{}});

        a.expect_eq(css::MediaQuery::parse("only print"),
                css::MediaQuery{css::MediaQuery::Type{.type = css::MediaType::Print}} //
        );
        a.expect_eq(css::MediaQuery::parse("only screen"),
                css::MediaQuery{css::MediaQuery::Type{.type = css::MediaType::Screen}} //
        );

        a.expect_eq(css::MediaQuery::parse("asdf"), std::nullopt);

        using Type = css::MediaQuery::Type;
        a.expect(Type{.type = css::MediaType::Print}.evaluate({.media_type = css::MediaType::Print}));
        a.expect(!Type{.type = css::MediaType::Print}.evaluate({.media_type = css::MediaType::Screen}));

        a.expect(Type{.type = css::MediaType::Screen}.evaluate({.media_type = css::MediaType::Screen}));
        a.expect(!Type{.type = css::MediaType::Screen}.evaluate({.media_type = css::MediaType::Print}));
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
    true_tests(s);
    prefers_color_scheme_tests(s);
    type_tests(s);
    width_tests(s);
    return s.run();
}
