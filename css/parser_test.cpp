#include "css/parse.h"

#include "etest/etest.h"

using namespace std::literals;
using etest::expect_true;

int main() {
    etest::test("parser: simple rule", [] {
        auto rules = css::parse("body { width: 50px; }"sv);
        expect_true(rules.size() == 1);

        auto body = rules[0];
        expect_true(body.selectors == std::vector{"body"s});
        expect_true(body.declarations.size() == 1);
        expect_true(body.declarations.at("width"s) == "50px"s);
    });

    etest::test("parser: multiple rules", [] {
        auto rules = css::parse("body { width: 50px; }\np { font-size: 8em; }"sv);
        expect_true(rules.size() == 2);

        auto body = rules[0];
        expect_true(body.selectors == std::vector{"body"s});
        expect_true(body.declarations.size() == 1);
        expect_true(body.declarations.at("width"s) == "50px"s);

        auto p = rules[1];
        expect_true(p.selectors == std::vector{"p"s});
        expect_true(p.declarations.size() == 1);
        expect_true(p.declarations.at("font-size"s) == "8em"s);
    });

    etest::test("parser: multiple selectors", [] {
        auto rules = css::parse("body, p { width: 50px; }"sv);
        expect_true(rules.size() == 1);

        auto body = rules[0];
        expect_true(body.selectors == std::vector{"body"s, "p"s});
        expect_true(body.declarations.size() == 1);
        expect_true(body.declarations.at("width"s) == "50px"s);
    });

    etest::test("parser: multiple declarations", [] {
        auto rules = css::parse("body { width: 50px; height: 300px; }"sv);
        expect_true(rules.size() == 1);

        auto body = rules[0];
        expect_true(body.selectors == std::vector{"body"s});
        expect_true(body.declarations.size() == 2);
        expect_true(body.declarations.at("width"s) == "50px"s);
        expect_true(body.declarations.at("height"s) == "300px"s);
    });

    etest::test("parser: class", [] {
        auto rules = css::parse(".cls { width: 50px; }"sv);
        expect_true(rules.size() == 1);

        auto body = rules[0];
        expect_true(body.selectors == std::vector{".cls"s});
        expect_true(body.declarations.size() == 1);
        expect_true(body.declarations.at("width"s) == "50px"s);
    });

    etest::test("parser: id", [] {
        auto rules = css::parse("#cls { width: 50px; }"sv);
        expect_true(rules.size() == 1);

        auto body = rules[0];
        expect_true(body.selectors == std::vector{"#cls"s});
        expect_true(body.declarations.size() == 1);
        expect_true(body.declarations.at("width"s) == "50px"s);
    });

    etest::test("parser: empty rule", [] {
        auto rules = css::parse("body {}"sv);
        expect_true(rules.size() == 1);

        auto body = rules[0];
        expect_true(body.selectors == std::vector{"body"s});
        expect_true(body.declarations.size() == 0);
    });

    etest::test("parser: no rules", [] {
        auto rules = css::parse(""sv);
        expect_true(rules.size() == 0);
    });

    return etest::run_all_tests();
}
