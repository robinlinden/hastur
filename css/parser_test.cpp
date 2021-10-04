// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2021 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

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

    etest::test("parser: media query", [] {
        auto rules = css::parse(R"(
                @media screen and (min-width: 900px) {
                    article { width: 50px; }
                    p { font-size: 9em; }
                }
                a { background-color: indigo; })"sv);
        require(rules.size() == 3);

        auto article = rules[0];
        expect(article.selectors == std::vector{"article"s});
        require(article.declarations.contains("width"));
        expect(article.declarations.at("width"s) == "50px"s);
        expect(article.media_query == "screen and (min-width: 900px)");

        auto p = rules[1];
        expect(p.selectors == std::vector{"p"s});
        require(p.declarations.contains("font-size"));
        expect(p.declarations.at("font-size"s) == "9em"s);
        expect(p.media_query == "screen and (min-width: 900px)");

        auto a = rules[2];
        expect(a.selectors == std::vector{"a"s});
        require(a.declarations.contains("background-color"));
        expect(a.declarations.at("background-color"s) == "indigo"s);
        expect(a.media_query.empty());
    });

    etest::test("parser: shorthand padding, one value", [] {
        auto rules = css::parse("p { padding: 10px; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 4);
        expect(body.declarations.at("padding-top"s) == "10px"s);
        expect(body.declarations.at("padding-bottom"s) == "10px"s);
        expect(body.declarations.at("padding-left"s) == "10px"s);
        expect(body.declarations.at("padding-right"s) == "10px"s);
    });

    etest::test("parser: shorthand padding, two values", [] {
        auto rules = css::parse("p { padding: 12em 36em; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 4);
        expect(body.declarations.at("padding-top"s) == "12em"s);
        expect(body.declarations.at("padding-bottom"s) == "12em"s);
        expect(body.declarations.at("padding-left"s) == "36em"s);
        expect(body.declarations.at("padding-right"s) == "36em"s);
    });

    etest::test("parser: shorthand padding, three values", [] {
        auto rules = css::parse("p { padding: 12px 36px 52px; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 4);
        expect(body.declarations.at("padding-top"s) == "12px"s);
        expect(body.declarations.at("padding-bottom"s) == "52px"s);
        expect(body.declarations.at("padding-left"s) == "36px"s);
        expect(body.declarations.at("padding-right"s) == "36px"s);
    });

    etest::test("parser: shorthand padding, four values", [] {
        auto rules = css::parse("p { padding: 12px 36px 52px 2px; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 4);
        expect(body.declarations.at("padding-top"s) == "12px"s);
        expect(body.declarations.at("padding-right"s) == "36px"s);
        expect(body.declarations.at("padding-bottom"s) == "52px"s);
        expect(body.declarations.at("padding-left"s) == "2px"s);
    });

    etest::test("parser: shorthand padding overridden", [] {
        auto rules = css::parse(R"(
                p {
                    padding: 10px;
                    padding-top: 15px;
                    padding-left: 25px;
                })"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 4);
        expect(body.declarations.at("padding-top"s) == "15px"s);
        expect(body.declarations.at("padding-bottom"s) == "10px"s);
        expect(body.declarations.at("padding-left"s) == "25px"s);
        expect(body.declarations.at("padding-right"s) == "10px"s);
    });

    etest::test("parser: override padding with shorthand", [] {
        auto rules = css::parse(R"(
                p {
                    padding-bottom: 5px;
                    padding-left: 25px;
                    padding: 12px 40px;
                })"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 4);
        expect(body.declarations.at("padding-top"s) == "12px"s);
        expect(body.declarations.at("padding-bottom"s) == "12px"s);
        expect(body.declarations.at("padding-left"s) == "40px"s);
        expect(body.declarations.at("padding-right"s) == "40px"s);
    });

    etest::test("parser: shorthand font with only size and generic font family", [] {
        auto rules = css::parse("p { font: 1.5em sans-serif; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 7);
        expect(body.declarations.at("font-family"s) == "sans-serif"s);
        expect(body.declarations.at("font-size"s) == "1.5em"s);
        expect(body.declarations.at("font-stretch"s) == "normal"s);
        expect(body.declarations.at("font-style"s) == "normal"s);
        expect(body.declarations.at("font-variant"s) == "normal"s);
        expect(body.declarations.at("font-weight"s) == "normal"s);
        expect(body.declarations.at("line-height"s) == "normal"s);
    });

    etest::test("parser: shorthand font with size, line height, and generic font family", [] {
        auto rules = css::parse("p { font: 10%/2.5 monospace; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 7);
        expect(body.declarations.at("font-family"s) == "monospace"s);
        expect(body.declarations.at("font-size"s) == "10%"s);
        expect(body.declarations.at("font-stretch"s) == "normal"s);
        expect(body.declarations.at("font-style"s) == "normal"s);
        expect(body.declarations.at("font-variant"s) == "normal"s);
        expect(body.declarations.at("font-weight"s) == "normal"s);
        expect(body.declarations.at("line-height"s) == "2.5"s);
    });

    etest::test("parser: shorthand font with size, line height, and generic font family", [] {
        auto rules = css::parse("p { font: 10%/2.5 monospace; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 7);
        expect(body.declarations.at("font-family"s) == "monospace"s);
        expect(body.declarations.at("font-size"s) == "10%"s);
        expect(body.declarations.at("font-stretch"s) == "normal"s);
        expect(body.declarations.at("font-style"s) == "normal"s);
        expect(body.declarations.at("font-variant"s) == "normal"s);
        expect(body.declarations.at("font-weight"s) == "normal"s);
        expect(body.declarations.at("line-height"s) == "2.5"s);
    });

    etest::test("parser: shorthand font with absolute size, line height, and font family", [] {
        auto rules = css::parse(R"(p { font: x-large/110% "New Century Schoolbook", serif; })"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 7);
        expect(body.declarations.at("font-family"s) == "\"New Century Schoolbook\", serif"s);
        expect(body.declarations.at("font-size"s) == "x-large"s);
        expect(body.declarations.at("font-stretch"s) == "normal"s);
        expect(body.declarations.at("font-style"s) == "normal"s);
        expect(body.declarations.at("font-variant"s) == "normal"s);
        expect(body.declarations.at("font-weight"s) == "normal"s);
        expect(body.declarations.at("line-height"s) == "110%"s);
    });

    etest::test("parser: shorthand font with italic font style", [] {
        auto rules = css::parse(R"(p { font: italic 120% "Helvetica Neue", serif; })"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 7);
        expect(body.declarations.at("font-family"s) == "\"Helvetica Neue\", serif"s);
        expect(body.declarations.at("font-size"s) == "120%"s);
        expect(body.declarations.at("font-stretch"s) == "normal"s);
        expect(body.declarations.at("font-style"s) == "italic"s);
        expect(body.declarations.at("font-variant"s) == "normal"s);
        expect(body.declarations.at("font-weight"s) == "normal"s);
        expect(body.declarations.at("line-height"s) == "normal"s);
    });

    etest::test("parser: shorthand font with oblique font style", [] {
        auto rules = css::parse(R"(p { font: oblique 12pt "Helvetica Neue", serif; })"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 7);
        expect(body.declarations.at("font-family"s) == "\"Helvetica Neue\", serif"s);
        expect(body.declarations.at("font-size"s) == "12pt"s);
        expect(body.declarations.at("font-stretch"s) == "normal"s);
        expect(body.declarations.at("font-style"s) == "oblique"s);
        expect(body.declarations.at("font-variant"s) == "normal"s);
        expect(body.declarations.at("font-weight"s) == "normal"s);
        expect(body.declarations.at("line-height"s) == "normal"s);
    });

    etest::test("parser: shorthand font with font style oblique with angle", [] {
        auto rules = css::parse("p { font: oblique 25deg 10px serif; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 7);
        expect(body.declarations.at("font-family"s) == "serif"s);
        expect(body.declarations.at("font-size"s) == "10px"s);
        expect(body.declarations.at("font-stretch"s) == "normal"s);
        expect(body.declarations.at("font-style"s) == "oblique 25deg"s);
        expect(body.declarations.at("font-variant"s) == "normal"s);
        expect(body.declarations.at("font-weight"s) == "normal"s);
        expect(body.declarations.at("line-height"s) == "normal"s);
    });

    etest::test("parser: shorthand font with bold font weight", [] {
        auto rules = css::parse("p { font: italic bold 20em/50% serif; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 7);
        expect(body.declarations.at("font-family"s) == "serif"s);
        expect(body.declarations.at("font-size"s) == "20em"s);
        expect(body.declarations.at("font-stretch"s) == "normal"s);
        expect(body.declarations.at("font-style"s) == "italic"s);
        expect(body.declarations.at("font-variant"s) == "normal"s);
        expect(body.declarations.at("font-weight"s) == "bold"s);
        expect(body.declarations.at("line-height"s) == "50%"s);
    });

    etest::test("parser: shorthand font with bolder font weight", [] {
        auto rules = css::parse("p { font: normal bolder 100px serif; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 7);
        expect(body.declarations.at("font-family"s) == "serif"s);
        expect(body.declarations.at("font-size"s) == "100px"s);
        expect(body.declarations.at("font-stretch"s) == "normal"s);
        expect(body.declarations.at("font-style"s) == "normal"s);
        expect(body.declarations.at("font-variant"s) == "normal"s);
        expect(body.declarations.at("font-weight"s) == "bolder"s);
        expect(body.declarations.at("line-height"s) == "normal"s);
    });

    etest::test("parser: shorthand font with lighter font weight", [] {
        auto rules = css::parse("p { font: lighter 100px serif; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 7);
        expect(body.declarations.at("font-family"s) == "serif"s);
        expect(body.declarations.at("font-size"s) == "100px"s);
        expect(body.declarations.at("font-stretch"s) == "normal"s);
        expect(body.declarations.at("font-style"s) == "normal"s);
        expect(body.declarations.at("font-variant"s) == "normal"s);
        expect(body.declarations.at("font-weight"s) == "lighter"s);
        expect(body.declarations.at("line-height"s) == "normal"s);
    });

    etest::test("parser: shorthand font with 1000 font weight", [] {
        auto rules = css::parse("p { font: 1000 oblique 100px serif; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 7);
        expect(body.declarations.at("font-family"s) == "serif"s);
        expect(body.declarations.at("font-size"s) == "100px"s);
        expect(body.declarations.at("font-stretch"s) == "normal"s);
        expect(body.declarations.at("font-style"s) == "oblique"s);
        expect(body.declarations.at("font-variant"s) == "normal"s);
        expect(body.declarations.at("font-weight"s) == "1000"s);
        expect(body.declarations.at("line-height"s) == "normal"s);
    });

    etest::test("parser: shorthand font with 550 font weight", [] {
        auto rules = css::parse("p { font: italic 550 100px serif; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 7);
        expect(body.declarations.at("font-family"s) == "serif"s);
        expect(body.declarations.at("font-size"s) == "100px"s);
        expect(body.declarations.at("font-stretch"s) == "normal"s);
        expect(body.declarations.at("font-style"s) == "italic"s);
        expect(body.declarations.at("font-variant"s) == "normal"s);
        expect(body.declarations.at("font-weight"s) == "550"s);
        expect(body.declarations.at("line-height"s) == "normal"s);
    });

    etest::test("parser: shorthand font with 1 font weight", [] {
        auto rules = css::parse("p { font: oblique 1 100px serif; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 7);
        expect(body.declarations.at("font-family"s) == "serif"s);
        expect(body.declarations.at("font-size"s) == "100px"s);
        expect(body.declarations.at("font-stretch"s) == "normal"s);
        expect(body.declarations.at("font-style"s) == "oblique"s);
        expect(body.declarations.at("font-variant"s) == "normal"s);
        expect(body.declarations.at("font-weight"s) == "1"s);
        expect(body.declarations.at("line-height"s) == "normal"s);
    });

    etest::test("parser: shorthand font with smal1-caps font variant", [] {
        auto rules = css::parse("p { font: small-caps 900 100px serif; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 7);
        expect(body.declarations.at("font-family"s) == "serif"s);
        expect(body.declarations.at("font-size"s) == "100px"s);
        expect(body.declarations.at("font-stretch"s) == "normal"s);
        expect(body.declarations.at("font-style"s) == "normal"s);
        expect(body.declarations.at("font-variant"s) == "small-caps"s);
        expect(body.declarations.at("font-weight"s) == "900"s);
        expect(body.declarations.at("line-height"s) == "normal"s);
    });

    etest::test("parser: shorthand font with condensed font stretch", [] {
        auto rules = css::parse(R"(p { font: condensed oblique 25deg 753 12pt "Helvetica Neue", serif; })"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 7);
        expect(body.declarations.at("font-family"s) == "\"Helvetica Neue\", serif"s);
        expect(body.declarations.at("font-size"s) == "12pt"s);
        expect(body.declarations.at("font-stretch"s) == "condensed"s);
        expect(body.declarations.at("font-style"s) == "oblique 25deg"s);
        expect(body.declarations.at("font-variant"s) == "normal"s);
        expect(body.declarations.at("font-weight"s) == "753"s);
        expect(body.declarations.at("line-height"s) == "normal"s);
    });

    etest::test("parser: shorthand font with exapnded font stretch", [] {
        auto rules = css::parse("p { font: italic expanded bold xx-smal/80% monospace; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 7);
        expect(body.declarations.at("font-family"s) == "monospace"s);
        expect(body.declarations.at("font-size"s) == "xx-smal"s);
        expect(body.declarations.at("font-stretch"s) == "expanded"s);
        expect(body.declarations.at("font-style"s) == "italic"s);
        expect(body.declarations.at("font-variant"s) == "normal"s);
        expect(body.declarations.at("font-weight"s) == "bold"s);
        expect(body.declarations.at("line-height"s) == "80%"s);
    });

    etest::test("parser: shorthand font with ultra-exapnded font stretch", [] {
        auto rules = css::parse("p { font: small-caps italic ultra-expanded bold medium Arial, monospace; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 7);
        expect(body.declarations.at("font-family"s) == "Arial, monospace"s);
        expect(body.declarations.at("font-size"s) == "medium"s);
        expect(body.declarations.at("font-stretch"s) == "ultra-expanded"s);
        expect(body.declarations.at("font-style"s) == "italic"s);
        expect(body.declarations.at("font-variant"s) == "small-caps"s);
        expect(body.declarations.at("font-weight"s) == "bold"s);
        expect(body.declarations.at("line-height"s) == "normal"s);
    });

    return etest::run_all_tests();
}
