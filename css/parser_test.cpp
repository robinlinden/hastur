// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2021 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "css/parse.h"

#include "etest/etest.h"

#include <algorithm>
#include <iterator>

#include <fmt/format.h>

#include <array>

using namespace std::literals;
using etest::expect;
using etest::expect_eq;
using etest::require;
using etest::require_eq;

namespace {

[[maybe_unused]] std::ostream &operator<<(std::ostream &os, std::vector<std::string> const &vec) {
    std::copy(cbegin(vec), cend(vec), std::ostream_iterator<std::string const &>(os, " "));
    return os;
}

auto const initial_font_values = std::map<std::string, std::string, std::less<>>{{"font-stretch", "normal"},
        {"font-variant", "normal"},
        {"font-weight", "normal"},
        {"line-height", "normal"},
        {"font-style", "normal"},
        {"font-size-adjust", "none"},
        {"font-kerning", "auto"},
        {"font-feature-settings", "normal"},
        {"font-language-override", "normal"},
        {"font-optical-sizing", "auto"},
        {"font-variation-settings", "normal"},
        {"font-palette", "normal"},
        {"font-variant-alternates", "normal"},
        {"font-variant-caps", "normal"},
        {"font-variant-ligatures", "normal"},
        {"font-variant-numeric", "normal"},
        {"font-variant-position", "normal"},
        {"font-variant-east-asian", "normal"}};

bool check_initial_font_values(std::map<std::string, std::string, std::less<>> declarations) {
    for (auto [property, value] : declarations) {
        auto it = initial_font_values.find(property);
        if (it != cend(initial_font_values) && it->second != value) {
            return false;
        }
    }
    return true;
}

template<class KeyT, class ValueT>
ValueT get_and_erase(std::map<KeyT, ValueT, std::less<>> &map, KeyT key) {
    ValueT value = map[key];
    map.erase(key);
    return value;
}

} // namespace

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
        expect(body.declarations.empty());
    });

    etest::test("parser: no rules", [] {
        auto rules = css::parse(""sv);
        expect(rules.empty());
    });

    etest::test("parser: top-level comments", [] {
        auto rules = css::parse("body { width: 50px; }/* comment. */ p { font-size: 8em; } /* comment. */"sv);
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

    etest::test("parser: comments almost everywhere", [] {
        // body { width: 50px; } p { padding: 8em 4em; } with comments added everywhere currently supported.
        auto rules = css::parse(R"(/**/body /**/{/**/width:50px;/**/}/*
                */p /**/{/**/padding:/**/8em 4em;/**//**/}/**/)"sv);
        // TODO(robinlinden): Support comments in more places.
        // auto rules = css::parse(R"(/**/body/**/{/**/width/**/:/**/50px/**/;/**/}/*
        //         */p/**/{/**/padding/**/:/**/8em/**/4em/**/;/**//**/}/**/)"sv);
        require_eq(rules.size(), 2UL);

        auto body = rules[0];
        expect_eq(body.selectors, std::vector{"body"s});
        expect_eq(body.declarations.size(), 1UL);
        expect_eq(body.declarations.at("width"s), "50px"s);

        auto p = rules[1];
        expect_eq(p.selectors, std::vector{"p"s});
        expect_eq(p.declarations.size(), 4UL);
        expect_eq(p.declarations.at("padding-top"s), "8em"s);
        expect_eq(p.declarations.at("padding-bottom"s), "8em"s);
        expect_eq(p.declarations.at("padding-left"s), "4em"s);
        expect_eq(p.declarations.at("padding-right"s), "4em"s);
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

    etest::test("parser: minified media query", [] {
        auto rules = css::parse("@media(prefers-color-scheme: dark){p{font-size:10px;}}");
        require_eq(rules.size(), std::size_t{1});
        auto const &rule = rules[0];
        expect_eq(rule.media_query, "(prefers-color-scheme: dark)");
        expect_eq(rule.selectors, std::vector{"p"s});
        require_eq(rule.declarations.size(), std::size_t{1});
        expect_eq(rule.declarations.at("font-size"), "10px");
    });

    auto box_shorthand_one_value = [](std::string property, std::string value, std::string post_fix = "") {
        return [=] {
            auto rules = css::parse(fmt::format("p {{ {}: {}; }}"sv, property, value));
            require(rules.size() == 1);

            auto body = rules[0];
            expect(body.declarations.size() == 4);
            expect(body.declarations.at(fmt::format("{}-top{}", property, post_fix)) == value);
            expect(body.declarations.at(fmt::format("{}-bottom{}", property, post_fix)) == value);
            expect(body.declarations.at(fmt::format("{}-left{}", property, post_fix)) == value);
            expect(body.declarations.at(fmt::format("{}-right{}", property, post_fix)) == value);
        };
    };

    {
        std::string size_value{"10px"};
        etest::test("parser: shorthand padding, one value", box_shorthand_one_value("padding", size_value));
        etest::test("parser: shorthand margin, one value", box_shorthand_one_value("margin", size_value));

        std::string border_style{"dashed"};
        etest::test("parser: shorthand border-style, one value",
                box_shorthand_one_value("border-style", border_style, "-style"));
    }

    auto box_shorthand_two_values =
            [](std::string property, std::array<std::string, 2> values, std::string post_fix = "") {
                return [=] {
                    auto rules = css::parse(fmt::format("p {{ {}: {} {}; }}"sv, property, values[0], values[1]));
                    require(rules.size() == 1);

                    auto body = rules[0];
                    expect(body.declarations.size() == 4);
                    expect(body.declarations.at(fmt::format("{}-top{}", property, post_fix)) == values[0]);
                    expect(body.declarations.at(fmt::format("{}-bottom{}", property, post_fix)) == values[0]);
                    expect(body.declarations.at(fmt::format("{}-left{}", property, post_fix)) == values[1]);
                    expect(body.declarations.at(fmt::format("{}-right{}", property, post_fix)) == values[1]);
                };
            };

    {
        auto size_values = std::array{"12em"s, "36em"s};
        etest::test("parser: shorthand padding, two values", box_shorthand_two_values("padding", size_values));
        etest::test("parser: shorthand margin, two values", box_shorthand_two_values("margin", size_values));

        auto border_styles = std::array{"dashed"s, "solid"s};
        etest::test("parser: shorthand border-style, two values",
                box_shorthand_two_values("border-style", border_styles, "-style"));
    }

    auto box_shorthand_three_values = [](std::string property,
                                              std::array<std::string, 3> values,
                                              std::string post_fix = "") {
        return [=] {
            auto rules = css::parse(fmt::format("p {{ {}: {} {} {}; }}"sv, property, values[0], values[1], values[2]));
            require(rules.size() == 1);

            auto body = rules[0];
            expect(body.declarations.size() == 4);
            expect(body.declarations.at(fmt::format("{}-top{}", property, post_fix)) == values[0]);
            expect(body.declarations.at(fmt::format("{}-bottom{}", property, post_fix)) == values[2]);
            expect(body.declarations.at(fmt::format("{}-left{}", property, post_fix)) == values[1]);
            expect(body.declarations.at(fmt::format("{}-right{}", property, post_fix)) == values[1]);
        };
    };

    {
        auto size_values = std::array{"12em"s, "36em"s, "52px"s};
        etest::test("parser: shorthand padding, three values", box_shorthand_three_values("padding", size_values));
        etest::test("parser: shorthand margin, three values", box_shorthand_three_values("margin", size_values));

        auto border_styles = std::array{"groove"s, "dashed"s, "solid"s};
        etest::test("parser: shorthand border-style, three values",
                box_shorthand_three_values("border-style", border_styles, "-style"));
    }

    auto box_shorthand_four_values =
            [](std::string property, std::array<std::string, 4> values, std::string post_fix = "") {
                return [=] {
                    auto rules = css::parse(fmt::format(
                            "p {{ {}: {} {} {} {}; }}"sv, property, values[0], values[1], values[2], values[3]));
                    require(rules.size() == 1);

                    auto body = rules[0];
                    expect(body.declarations.size() == 4);
                    expect(body.declarations.at(fmt::format("{}-top{}", property, post_fix)) == values[0]);
                    expect(body.declarations.at(fmt::format("{}-bottom{}", property, post_fix)) == values[2]);
                    expect(body.declarations.at(fmt::format("{}-left{}", property, post_fix)) == values[3]);
                    expect(body.declarations.at(fmt::format("{}-right{}", property, post_fix)) == values[1]);
                };
            };

    {
        auto size_values = std::array{"12px"s, "36px"s, "52px"s, "2"s};
        etest::test("parser: shorthand padding, four values", box_shorthand_four_values("padding", size_values));
        etest::test("parser: shorthand margin, four values", box_shorthand_four_values("margin", size_values));

        auto border_styles = std::array{"groove"s, "dashed"s, "solid"s, "dotted"s};
        etest::test("parser: shorthand border-style, four values",
                box_shorthand_four_values("border-style", border_styles, "-style"));
    }

    auto box_shorthand_overridden =
            [](std::string property, std::array<std::string, 3> values, std::string post_fix = "") {
                return [=] {
                    auto rules = css::parse(fmt::format(R"(
                            p {{
                               {0}: {2};
                               {0}-top{1}: {3};
                               {0}-left{1}: {4};
                            }})"sv,
                            property,
                            post_fix,
                            values[0],
                            values[1],
                            values[2]));
                    require(rules.size() == 1);

                    auto body = rules[0];
                    expect(body.declarations.size() == 4);
                    expect(body.declarations.at(fmt::format("{}-top{}", property, post_fix)) == values[1]);
                    expect(body.declarations.at(fmt::format("{}-bottom{}", property, post_fix)) == values[0]);
                    expect(body.declarations.at(fmt::format("{}-left{}", property, post_fix)) == values[2]);
                    expect(body.declarations.at(fmt::format("{}-right{}", property, post_fix)) == values[0]);
                };
            };

    {
        auto size_values = std::array{"10px"s, "15px"s, "25px"s};
        etest::test("parser: shorthand padding overridden", box_shorthand_overridden("padding", size_values));
        etest::test("parser: shorthand margin overridden", box_shorthand_overridden("margin", size_values));

        auto border_styles = std::array{"dashed"s, "solid"s, "dotted"s};
        etest::test("parser: shorthand border-style overridden",
                box_shorthand_overridden("border-style", border_styles, "-style"));
    }

    auto box_override_with_shorthand =
            [](std::string property, std::array<std::string, 4> values, std::string post_fix = "") {
                return [=] {
                    auto rules = css::parse(fmt::format(R"(
                            p {{
                               {0}-bottom{1}: {2};
                               {0}-left{1}: {3};
                               {0}: {4} {5};
                            }})"sv,
                            property,
                            post_fix,
                            values[0],
                            values[1],
                            values[2],
                            values[3]));
                    require(rules.size() == 1);

                    auto body = rules[0];
                    expect(body.declarations.size() == 4);
                    expect(body.declarations.at(fmt::format("{}-top{}", property, post_fix)) == values[2]);
                    expect(body.declarations.at(fmt::format("{}-bottom{}", property, post_fix)) == values[2]);
                    expect(body.declarations.at(fmt::format("{}-left{}", property, post_fix)) == values[3]);
                    expect(body.declarations.at(fmt::format("{}-right{}", property, post_fix)) == values[3]);
                };
            };

    {
        auto size_values = std::array{"5px"s, "25px"s, "12px"s, "40px"s};
        etest::test("parser: override padding with shorthand", box_override_with_shorthand("padding", size_values));
        etest::test("parser: override margin with shorthand", box_override_with_shorthand("margin", size_values));

        auto border_styles = std::array{"dashed"s, "solid"s, "hidden"s, "dotted"s};
        etest::test("parser: override border-style with shorthand",
                box_override_with_shorthand("border-style", border_styles, "-style"));
    }

    etest::test("parser: shorthand font with only size and generic font family", [] {
        auto rules = css::parse("p { font: 1.5em sans-serif; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 20);
        expect(get_and_erase(body.declarations, "font-family"s) == "sans-serif"s);
        expect(get_and_erase(body.declarations, "font-size"s) == "1.5em"s);
        expect(check_initial_font_values(body.declarations));
    });

    etest::test("parser: shorthand font with size, line height, and generic font family", [] {
        auto rules = css::parse("p { font: 10%/2.5 monospace; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 20);
        expect(get_and_erase(body.declarations, "font-family"s) == "monospace"s);
        expect(get_and_erase(body.declarations, "font-size"s) == "10%"s);
        expect(get_and_erase(body.declarations, "line-height"s) == "2.5"s);
        expect(check_initial_font_values(body.declarations));
    });

    etest::test("parser: shorthand font with absolute size, line height, and font family", [] {
        auto rules = css::parse(R"(p { font: x-large/110% "New Century Schoolbook", serif; })"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 20);
        expect(get_and_erase(body.declarations, "font-family"s) == "\"New Century Schoolbook\", serif"s);
        expect(get_and_erase(body.declarations, "font-size"s) == "x-large"s);
        expect(get_and_erase(body.declarations, "line-height"s) == "110%"s);
        expect(check_initial_font_values(body.declarations));
    });

    etest::test("parser: shorthand font with italic font style", [] {
        auto rules = css::parse(R"(p { font: italic 120% "Helvetica Neue", serif; })"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 20);
        expect(get_and_erase(body.declarations, "font-family"s) == "\"Helvetica Neue\", serif"s);
        expect(get_and_erase(body.declarations, "font-size"s) == "120%"s);
        expect(get_and_erase(body.declarations, "font-style"s) == "italic"s);
        expect(check_initial_font_values(body.declarations));
    });

    etest::test("parser: shorthand font with oblique font style", [] {
        auto rules = css::parse(R"(p { font: oblique 12pt "Helvetica Neue", serif; })"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 20);
        expect(get_and_erase(body.declarations, "font-family"s) == "\"Helvetica Neue\", serif"s);
        expect(get_and_erase(body.declarations, "font-size"s) == "12pt"s);
        expect(get_and_erase(body.declarations, "font-style"s) == "oblique"s);
        expect(check_initial_font_values(body.declarations));
    });

    etest::test("parser: shorthand font with font style oblique with angle", [] {
        auto rules = css::parse("p { font: oblique 25deg 10px serif; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 20);
        expect(get_and_erase(body.declarations, "font-family"s) == "serif"s);
        expect(get_and_erase(body.declarations, "font-size"s) == "10px"s);
        expect(get_and_erase(body.declarations, "font-style"s) == "oblique 25deg"s);
        expect(check_initial_font_values(body.declarations));
    });

    etest::test("parser: shorthand font with bold font weight", [] {
        auto rules = css::parse("p { font: italic bold 20em/50% serif; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 20);
        expect(get_and_erase(body.declarations, "font-family"s) == "serif"s);
        expect(get_and_erase(body.declarations, "font-size"s) == "20em"s);
        expect(get_and_erase(body.declarations, "font-style"s) == "italic"s);
        expect(get_and_erase(body.declarations, "font-weight"s) == "bold"s);
        expect(get_and_erase(body.declarations, "line-height"s) == "50%"s);
        expect(check_initial_font_values(body.declarations));
    });

    etest::test("parser: shorthand font with bolder font weight", [] {
        auto rules = css::parse("p { font: normal bolder 100px serif; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 20);
        expect(get_and_erase(body.declarations, "font-family"s) == "serif"s);
        expect(get_and_erase(body.declarations, "font-size"s) == "100px"s);
        expect(get_and_erase(body.declarations, "font-weight"s) == "bolder"s);
        expect(check_initial_font_values(body.declarations));
    });

    etest::test("parser: shorthand font with lighter font weight", [] {
        auto rules = css::parse("p { font: lighter 100px serif; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 20);
        expect(get_and_erase(body.declarations, "font-family"s) == "serif"s);
        expect(get_and_erase(body.declarations, "font-size"s) == "100px"s);
        expect(get_and_erase(body.declarations, "font-weight"s) == "lighter"s);
        expect(check_initial_font_values(body.declarations));
    });

    etest::test("parser: shorthand font with 1000 font weight", [] {
        auto rules = css::parse("p { font: 1000 oblique 100px serif; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 20);
        expect(get_and_erase(body.declarations, "font-family"s) == "serif"s);
        expect(get_and_erase(body.declarations, "font-size"s) == "100px"s);
        expect(get_and_erase(body.declarations, "font-style"s) == "oblique"s);
        expect(get_and_erase(body.declarations, "font-weight"s) == "1000"s);
        expect(check_initial_font_values(body.declarations));
    });

    etest::test("parser: shorthand font with 550 font weight", [] {
        auto rules = css::parse("p { font: italic 550 100px serif; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 20);
        expect(get_and_erase(body.declarations, "font-family"s) == "serif"s);
        expect(get_and_erase(body.declarations, "font-size"s) == "100px"s);
        expect(get_and_erase(body.declarations, "font-style"s) == "italic"s);
        expect(get_and_erase(body.declarations, "font-weight"s) == "550"s);
        expect(check_initial_font_values(body.declarations));
    });

    etest::test("parser: shorthand font with 1 font weight", [] {
        auto rules = css::parse("p { font: oblique 1 100px serif; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 20);
        expect(get_and_erase(body.declarations, "font-family"s) == "serif"s);
        expect(get_and_erase(body.declarations, "font-size"s) == "100px"s);
        expect(get_and_erase(body.declarations, "font-style"s) == "oblique"s);
        expect(get_and_erase(body.declarations, "font-weight"s) == "1"s);
        expect(check_initial_font_values(body.declarations));
    });

    etest::test("parser: shorthand font with smal1-caps font variant", [] {
        auto rules = css::parse("p { font: small-caps 900 100px serif; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 20);
        expect(get_and_erase(body.declarations, "font-family"s) == "serif"s);
        expect(get_and_erase(body.declarations, "font-size"s) == "100px"s);
        expect(get_and_erase(body.declarations, "font-variant"s) == "small-caps"s);
        expect(get_and_erase(body.declarations, "font-weight"s) == "900"s);
        expect(check_initial_font_values(body.declarations));
    });

    etest::test("parser: shorthand font with condensed font stretch", [] {
        auto rules = css::parse(R"(p { font: condensed oblique 25deg 753 12pt "Helvetica Neue", serif; })"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 20);
        expect(get_and_erase(body.declarations, "font-family"s) == "\"Helvetica Neue\", serif"s);
        expect(get_and_erase(body.declarations, "font-size"s) == "12pt"s);
        expect(get_and_erase(body.declarations, "font-stretch"s) == "condensed"s);
        expect(get_and_erase(body.declarations, "font-style"s) == "oblique 25deg"s);
        expect(get_and_erase(body.declarations, "font-weight"s) == "753"s);
        expect(check_initial_font_values(body.declarations));
    });

    etest::test("parser: shorthand font with exapnded font stretch", [] {
        auto rules = css::parse("p { font: italic expanded bold xx-smal/80% monospace; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 20);
        expect(get_and_erase(body.declarations, "font-family"s) == "monospace"s);
        expect(get_and_erase(body.declarations, "font-size"s) == "xx-smal"s);
        expect(get_and_erase(body.declarations, "font-stretch"s) == "expanded"s);
        expect(get_and_erase(body.declarations, "font-style"s) == "italic"s);
        expect(get_and_erase(body.declarations, "font-weight"s) == "bold"s);
        expect(get_and_erase(body.declarations, "line-height"s) == "80%"s);
        expect(check_initial_font_values(body.declarations));
    });

    etest::test("parser: shorthand font with ultra-exapnded font stretch", [] {
        auto rules = css::parse("p { font: small-caps italic ultra-expanded bold medium Arial, monospace; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 20);
        expect(get_and_erase(body.declarations, "font-family"s) == "Arial, monospace"s);
        expect(get_and_erase(body.declarations, "font-size"s) == "medium"s);
        expect(get_and_erase(body.declarations, "font-stretch"s) == "ultra-expanded"s);
        expect(get_and_erase(body.declarations, "font-style"s) == "italic"s);
        expect(get_and_erase(body.declarations, "font-variant"s) == "small-caps"s);
        expect(get_and_erase(body.declarations, "font-weight"s) == "bold"s);
        expect(check_initial_font_values(body.declarations));
    });

    etest::test("parser: @keyframes doesn't crash the parser", [] {
        auto css = R"(
            @keyframes toast-spinner {
                from {
                    transform: rotate(0deg)
                }

                to {
                    transform: rotate(360deg)
                }
            })"sv;

        // No rules produced (yet!) since this isn't handled aside from not crashing.
        auto rules = css::parse(css);
        expect(rules.empty());
    });

    return etest::run_all_tests();
}
