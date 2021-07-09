#include "css/parse.h"

#include "etest/etest.h"

using namespace std::literals;
using etest::expect;
using etest::require;

int main() {
    etest::test("parser: simple rule", [] {
        auto rules = css::parse("body { width: 50px; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.selectors == std::vector{"body"s});
        expect(body.declarations.size() == 1);
        expect(body.declarations.at("width"s) == "50px"s);
    });

    etest::test("parser: minified", [] {
        auto rules = css::parse("body{width:50px;font:inherit}head,p{display:none}"sv);
        require(rules.size() == 2);

        auto first = rules[0];
        expect(first.selectors == std::vector{"body"s});
        expect(first.declarations.size() == 2);
        expect(first.declarations.at("width"s) == "50px"s);
        expect(first.declarations.at("font"s) == "inherit"s);

        auto second = rules[1];
        expect(second.selectors == std::vector{"head"s, "p"s});
        expect(second.declarations.size() == 1);
        expect(second.declarations.at("display"s) == "none"s);
    });

    etest::test("parser: multiple rules", [] {
        auto rules = css::parse("body { width: 50px; }\np { font-size: 8em; }"sv);
        require(rules.size() == 2);

        auto body = rules[0];
        expect(body.selectors == std::vector{"body"s});
        expect(body.declarations.size() == 1);
        expect(body.declarations.at("width"s) == "50px"s);

        auto p = rules[1];
        expect(p.selectors == std::vector{"p"s});
        expect(p.declarations.size() == 1);
        expect(p.declarations.at("font-size"s) == "8em"s);
    });

    etest::test("parser: multiple selectors", [] {
        auto rules = css::parse("body, p { width: 50px; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.selectors == std::vector{"body"s, "p"s});
        expect(body.declarations.size() == 1);
        expect(body.declarations.at("width"s) == "50px"s);
    });

    etest::test("parser: multiple declarations", [] {
        auto rules = css::parse("body { width: 50px; height: 300px; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.selectors == std::vector{"body"s});
        expect(body.declarations.size() == 2);
        expect(body.declarations.at("width"s) == "50px"s);
        expect(body.declarations.at("height"s) == "300px"s);
    });

    etest::test("parser: class", [] {
        auto rules = css::parse(".cls { width: 50px; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.selectors == std::vector{".cls"s});
        expect(body.declarations.size() == 1);
        expect(body.declarations.at("width"s) == "50px"s);
    });

    etest::test("parser: id", [] {
        auto rules = css::parse("#cls { width: 50px; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.selectors == std::vector{"#cls"s});
        expect(body.declarations.size() == 1);
        expect(body.declarations.at("width"s) == "50px"s);
    });

    etest::test("parser: empty rule", [] {
        auto rules = css::parse("body {}"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.selectors == std::vector{"body"s});
        expect(body.declarations.size() == 0);
    });

    etest::test("parser: no rules", [] {
        auto rules = css::parse(""sv);
        expect(rules.size() == 0);
    });

    return etest::run_all_tests();
}
