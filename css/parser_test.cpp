#include "css/parse.h"

#include <catch2/catch.hpp>

using namespace std::literals;

namespace {

TEST_CASE("parser") {
    SECTION("simple rule") {
        auto rules = css::parse("body { width: 50px; }"sv);
        REQUIRE(rules.size() == 1);

        auto body = rules[0];
        REQUIRE(body.selectors == std::vector{"body"s});
        REQUIRE(body.declarations.size() == 1);
        REQUIRE(body.declarations.at("width"s) == "50px"s);
    }

    SECTION("multiple rules") {
        auto rules = css::parse("body { width: 50px; }\np { font-size: 8em; }"sv);
        REQUIRE(rules.size() == 2);

        auto body = rules[0];
        REQUIRE(body.selectors == std::vector{"body"s});
        REQUIRE(body.declarations.size() == 1);
        REQUIRE(body.declarations.at("width"s) == "50px"s);

        auto p = rules[1];
        REQUIRE(p.selectors == std::vector{"p"s});
        REQUIRE(p.declarations.size() == 1);
        REQUIRE(p.declarations.at("font-size"s) == "8em"s);
    }

    SECTION("multiple selectors") {
        auto rules = css::parse("body, p { width: 50px; }"sv);
        REQUIRE(rules.size() == 1);

        auto body = rules[0];
        REQUIRE(body.selectors == std::vector{"body"s, "p"s});
        REQUIRE(body.declarations.size() == 1);
        REQUIRE(body.declarations.at("width"s) == "50px"s);
    }

    SECTION("multiple declarations") {
        auto rules = css::parse("body { width: 50px; height: 300px; }"sv);
        REQUIRE(rules.size() == 1);

        auto body = rules[0];
        REQUIRE(body.selectors == std::vector{"body"s});
        REQUIRE(body.declarations.size() == 2);
        REQUIRE(body.declarations.at("width"s) == "50px"s);
        REQUIRE(body.declarations.at("height"s) == "300px"s);
    }

    SECTION("class") {
        auto rules = css::parse(".cls { width: 50px; }"sv);
        REQUIRE(rules.size() == 1);

        auto body = rules[0];
        REQUIRE(body.selectors == std::vector{".cls"s});
        REQUIRE(body.declarations.size() == 1);
        REQUIRE(body.declarations.at("width"s) == "50px"s);
    }

    SECTION("id") {
        auto rules = css::parse("#cls { width: 50px; }"sv);
        REQUIRE(rules.size() == 1);

        auto body = rules[0];
        REQUIRE(body.selectors == std::vector{"#cls"s});
        REQUIRE(body.declarations.size() == 1);
        REQUIRE(body.declarations.at("width"s) == "50px"s);
    }

    SECTION("empty rule") {
        auto rules = css::parse("body {}"sv);
        REQUIRE(rules.size() == 1);

        auto body = rules[0];
        REQUIRE(body.selectors == std::vector{"body"s});
        REQUIRE(body.declarations.size() == 0);
    }

    SECTION("no rules") {
        auto rules = css::parse(""sv);
        REQUIRE(rules.size() == 0);
    }
}

} // namespace
