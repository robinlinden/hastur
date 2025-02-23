// SPDX-FileCopyrightText: 2023-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "css/media_query.h"

#include "etest/etest2.h"

#include <cstring>
#include <optional>
#include <string>

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

    s.add_test("to_string: prefers-reduced-motion", [](etest::IActions &a) {
        a.expect_eq(css::to_string(css::MediaQuery::PrefersReducedMotion{}), //
                "prefers-reduced-motion: reduce");
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

void and_tests(etest::Suite &s) {
    s.add_test("and", [](etest::IActions &a) {
        auto query = css::MediaQuery::parse("(min-width: 300px) and (max-width: 400px)").value();

        a.expect_eq(query,
                css::MediaQuery{css::MediaQuery::And{{
                        css::MediaQuery{css::MediaQuery::Width{.min = 300}},
                        css::MediaQuery{css::MediaQuery::Width{.max = 400}},
                }}});

        a.expect(!query.evaluate({.window_width = 299}));
        a.expect(query.evaluate({.window_width = 300}));
        a.expect(query.evaluate({.window_width = 350}));
        a.expect(query.evaluate({.window_width = 400}));
        a.expect(!query.evaluate({.window_width = 401}));
    });

    s.add_test("and: false", [](etest::IActions &a) {
        auto query = css::MediaQuery::And{{
                css::MediaQuery{css::MediaQuery::False{}},
                css::MediaQuery{css::MediaQuery::Width{.max = 400}},
        }};

        a.expect(!query.evaluate({}));

        query = css::MediaQuery::And{{
                css::MediaQuery{css::MediaQuery::Width{.max = 400}},
                css::MediaQuery{css::MediaQuery::False{}},
        }};
        a.expect(!query.evaluate({}));
    });

    s.add_test("and: parse failures", [](etest::IActions &a) {
        a.expect_eq(css::MediaQuery::parse("(min-width: 300px) and blah"), std::nullopt);
        a.expect_eq(css::MediaQuery::parse("blah and (max-width: 400px)"), std::nullopt);
    });

    s.add_test("and: to_string", [](etest::IActions &a) {
        auto query = css::MediaQuery::And{{
                css::MediaQuery{css::MediaQuery::PrefersColorScheme{css::ColorScheme::Light}},
                css::MediaQuery{css::MediaQuery::Width{.max = 400}},
        }};

        a.expect_eq(css::to_string(css::MediaQuery{query}), "prefers-color-scheme: light and 0 <= width <= 400");
    });

    // In e.g. an MSVC debug build, this would consume all the stack after
    // trying to parse 599 ands, so let's parse 1000 of them.
    s.add_test("and: lots of ands", [](etest::IActions &a) {
        std::string query_str = "(width: 300px)";
        query_str.reserve(query_str.size() + std::strlen(" and (width: 300px)") * 1000);
        for (int i = 0; i < 1000; ++i) {
            query_str += " and (width: 300px)";
        }

        auto query = css::MediaQuery::parse(query_str).value();
        a.expect_eq(query.evaluate({.window_width = 300}), true);
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

void prefers_reduced_motion_tests(etest::Suite &s) {
    s.add_test("prefers-reduced-motion: reduce", [](etest::IActions &a) {
        a.expect_eq(css::MediaQuery::parse("(prefers-reduced-motion: reduce)"),
                css::MediaQuery{css::MediaQuery::PrefersReducedMotion{}});

        auto query = css::MediaQuery::PrefersReducedMotion{};
        a.expect(query.evaluate({.reduce_motion = css::ReduceMotion::Reduce}));
        a.expect(!query.evaluate({.reduce_motion = css::ReduceMotion::NoPreference}));
    });

    s.add_test("prefers-reduced-motion: no-preference", [](etest::IActions &a) {
        a.expect_eq(css::MediaQuery::parse("(prefers-reduced-motion: no-preference)"),
                css::MediaQuery{css::MediaQuery::False{}});
    });

    s.add_test("prefers-reduced-motion: no-preference", [](etest::IActions &a) {
        a.expect_eq(css::MediaQuery::parse("(prefers-reduced-motion: yasss)"), std::nullopt);
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
    and_tests(s);
    false_tests(s);
    true_tests(s);
    prefers_color_scheme_tests(s);
    prefers_reduced_motion_tests(s);
    type_tests(s);
    width_tests(s);
    return s.run();
}
