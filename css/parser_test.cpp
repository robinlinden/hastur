// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2021 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "css/parse.h"

#include "css/property_id.h"

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
    std::ranges::copy(vec, std::ostream_iterator<std::string const &>(os, " "));
    return os;
}

auto const initial_background_values =
        std::map<css::PropertyId, std::string>{{css::PropertyId::BackgroundImage, "none"},
                {css::PropertyId::BackgroundPosition, "0% 0%"},
                {css::PropertyId::BackgroundSize, "auto auto"},
                {css::PropertyId::BackgroundRepeat, "repeat"},
                {css::PropertyId::BackgroundOrigin, "padding-box"},
                {css::PropertyId::BackgroundClip, "border-box"},
                {css::PropertyId::BackgroundAttachment, "scroll"},
                {css::PropertyId::BackgroundColor, "transparent"}};

bool check_initial_background_values(std::map<css::PropertyId, std::string> const &declarations) {
    return std::ranges::all_of(declarations, [](auto const &decl) {
        auto it = initial_background_values.find(decl.first);
        return it != cend(initial_background_values) && it->second == decl.second;
    });
}

auto const initial_font_values = std::map<css::PropertyId, std::string>{{css::PropertyId::FontStretch, "normal"},
        {css::PropertyId::FontVariant, "normal"},
        {css::PropertyId::FontWeight, "normal"},
        {css::PropertyId::LineHeight, "normal"},
        {css::PropertyId::FontStyle, "normal"},
        {css::PropertyId::FontSizeAdjust, "none"},
        {css::PropertyId::FontKerning, "auto"},
        {css::PropertyId::FontFeatureSettings, "normal"},
        {css::PropertyId::FontLanguageOverride, "normal"},
        {css::PropertyId::FontOpticalSizing, "auto"},
        {css::PropertyId::FontVariationSettings, "normal"},
        {css::PropertyId::FontPalette, "normal"},
        {css::PropertyId::FontVariantAlternatives, "normal"},
        {css::PropertyId::FontVariantCaps, "normal"},
        {css::PropertyId::FontVariantLigatures, "normal"},
        {css::PropertyId::FontVariantNumeric, "normal"},
        {css::PropertyId::FontVariantPosition, "normal"},
        {css::PropertyId::FontVariantEastAsian, "normal"}};

bool check_initial_font_values(std::map<css::PropertyId, std::string> const &declarations) {
    return std::ranges::all_of(declarations, [](auto const &decl) {
        auto it = initial_font_values.find(decl.first);
        return it != cend(initial_font_values) && it->second == decl.second;
    });
}

template<class KeyT, class ValueT>
ValueT get_and_erase(
        std::map<KeyT, ValueT> &map, KeyT key, etest::source_location const &loc = etest::source_location::current()) {
    require(map.contains(key), {}, loc);
    ValueT value = map.at(key);
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
        expect(body.declarations.at(css::PropertyId::Width) == "50px"s);
    });

    etest::test("selector with spaces", [] {
        auto rules = css::parse("p a { color: green; }");
        expect_eq(rules,
                std::vector<css::Rule>{{
                        .selectors{{"p a"}},
                        .declarations{{css::PropertyId::Color, "green"}},
                }});
    });

    etest::test("parser: minified", [] {
        auto rules = css::parse("body{width:50px;font-family:inherit}head,p{display:none}"sv);
        require(rules.size() == 2);

        auto first = rules[0];
        expect(first.selectors == std::vector{"body"s});
        expect(first.declarations.size() == 2);
        expect(first.declarations.at(css::PropertyId::Width) == "50px"s);
        expect(first.declarations.at(css::PropertyId::FontFamily) == "inherit"s);

        auto second = rules[1];
        expect(second.selectors == std::vector{"head"s, "p"s});
        expect(second.declarations.size() == 1);
        expect(second.declarations.at(css::PropertyId::Display) == "none"s);
    });

    etest::test("parser: multiple rules", [] {
        auto rules = css::parse("body { width: 50px; }\np { font-size: 8em; }"sv);
        require(rules.size() == 2);

        auto body = rules[0];
        expect(body.selectors == std::vector{"body"s});
        expect(body.declarations.size() == 1);
        expect(body.declarations.at(css::PropertyId::Width) == "50px"s);

        auto p = rules[1];
        expect(p.selectors == std::vector{"p"s});
        expect(p.declarations.size() == 1);
        expect(p.declarations.at(css::PropertyId::FontSize) == "8em"s);
    });

    etest::test("parser: multiple selectors", [] {
        auto rules = css::parse("body, p { width: 50px; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.selectors == std::vector{"body"s, "p"s});
        expect(body.declarations.size() == 1);
        expect(body.declarations.at(css::PropertyId::Width) == "50px"s);
    });

    etest::test("parser: multiple declarations", [] {
        auto rules = css::parse("body { width: 50px; height: 300px; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.selectors == std::vector{"body"s});
        expect(body.declarations.size() == 2);
        expect(body.declarations.at(css::PropertyId::Width) == "50px"s);
        expect(body.declarations.at(css::PropertyId::Height) == "300px"s);
    });

    etest::test("parser: class", [] {
        auto rules = css::parse(".cls { width: 50px; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.selectors == std::vector{".cls"s});
        expect(body.declarations.size() == 1);
        expect(body.declarations.at(css::PropertyId::Width) == "50px"s);
    });

    etest::test("parser: id", [] {
        auto rules = css::parse("#cls { width: 50px; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.selectors == std::vector{"#cls"s});
        expect(body.declarations.size() == 1);
        expect(body.declarations.at(css::PropertyId::Width) == "50px"s);
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
        expect(body.declarations.at(css::PropertyId::Width) == "50px"s);

        auto p = rules[1];
        expect(p.selectors == std::vector{"p"s});
        expect(p.declarations.size() == 1);
        expect(p.declarations.at(css::PropertyId::FontSize) == "8em"s);
    });

    etest::test("parser: comments almost everywhere", [] {
        // body { width: 50px; } p { padding: 8em 4em; } with comments added everywhere currently supported.
        auto rules = css::parse(R"(/**/body {/**/width:50px;/**/}/*
                */p {/**/padding:/**/8em 4em;/**//**/}/**/)"sv);
        // TODO(robinlinden): Support comments in more places.
        // auto rules = css::parse(R"(/**/body/**/{/**/width/**/:/**/50px/**/;/**/}/*
        //         */p/**/{/**/padding/**/:/**/8em/**/4em/**/;/**//**/}/**/)"sv);
        require_eq(rules.size(), 2UL);

        auto body = rules[0];
        expect_eq(body.selectors, std::vector{"body"s});
        expect_eq(body.declarations.size(), 1UL);
        expect_eq(body.declarations.at(css::PropertyId::Width), "50px"s);

        auto p = rules[1];
        expect_eq(p.selectors, std::vector{"p"s});
        expect_eq(p.declarations.size(), 4UL);
        expect_eq(p.declarations.at(css::PropertyId::PaddingTop), "8em"s);
        expect_eq(p.declarations.at(css::PropertyId::PaddingBottom), "8em"s);
        expect_eq(p.declarations.at(css::PropertyId::PaddingLeft), "4em"s);
        expect_eq(p.declarations.at(css::PropertyId::PaddingRight), "4em"s);
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
        require(article.declarations.contains(css::PropertyId::Width));
        expect(article.declarations.at(css::PropertyId::Width) == "50px"s);
        expect(article.media_query == "screen and (min-width: 900px)");

        auto p = rules[1];
        expect(p.selectors == std::vector{"p"s});
        require(p.declarations.contains(css::PropertyId::FontSize));
        expect(p.declarations.at(css::PropertyId::FontSize) == "9em"s);
        expect(p.media_query == "screen and (min-width: 900px)");

        auto a = rules[2];
        expect(a.selectors == std::vector{"a"s});
        require(a.declarations.contains(css::PropertyId::BackgroundColor));
        expect(a.declarations.at(css::PropertyId::BackgroundColor) == "indigo"s);
        expect(a.media_query.empty());
    });

    etest::test("parser: minified media query", [] {
        auto rules = css::parse("@media(prefers-color-scheme: dark){p{font-size:10px;}}");
        require_eq(rules.size(), std::size_t{1});
        auto const &rule = rules[0];
        expect_eq(rule.media_query, "(prefers-color-scheme: dark)");
        expect_eq(rule.selectors, std::vector{"p"s});
        require_eq(rule.declarations.size(), std::size_t{1});
        expect_eq(rule.declarations.at(css::PropertyId::FontSize), "10px");
    });

    etest::test("parser: 2 media queries in a row", [] {
        auto rules = css::parse("@media screen { p { font-size: 1em; } } @media print { a { color: blue; } }");
        require_eq(rules.size(), std::size_t{2});
        expect_eq(rules[0],
                css::Rule{
                        .selectors{{"p"}}, .declarations{{css::PropertyId::FontSize, "1em"}}, .media_query{"screen"}});
        expect_eq(rules[1],
                css::Rule{.selectors{{"a"}}, .declarations{{css::PropertyId::Color, "blue"}}, .media_query{"print"}});
    });

    auto box_shorthand_one_value = [](std::string property, std::string value, std::string post_fix = "") {
        return [=]() mutable {
            auto rules = css::parse(fmt::format("p {{ {}: {}; }}"sv, property, value));
            require(rules.size() == 1);

            if (property == "border-style") {
                property = "border";
            }

            auto body = rules[0];
            expect(body.declarations.size() == 4);
            expect(body.declarations.at(css::property_id_from_string(fmt::format("{}-top{}", property, post_fix)))
                    == value);
            expect(body.declarations.at(css::property_id_from_string(fmt::format("{}-bottom{}", property, post_fix)))
                    == value);
            expect(body.declarations.at(css::property_id_from_string(fmt::format("{}-left{}", property, post_fix)))
                    == value);
            expect(body.declarations.at(css::property_id_from_string(fmt::format("{}-right{}", property, post_fix)))
                    == value);
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

    auto box_shorthand_two_values = [](std::string property,
                                            std::array<std::string, 2> values,
                                            std::string post_fix = "") {
        return [=]() mutable {
            auto rules = css::parse(fmt::format("p {{ {}: {} {}; }}"sv, property, values[0], values[1]));
            require(rules.size() == 1);

            if (property == "border-style") {
                property = "border";
            }

            auto body = rules[0];
            expect(body.declarations.size() == 4);
            expect(body.declarations.at(css::property_id_from_string(fmt::format("{}-top{}", property, post_fix)))
                    == values[0]);
            expect(body.declarations.at(css::property_id_from_string(fmt::format("{}-bottom{}", property, post_fix)))
                    == values[0]);
            expect(body.declarations.at(css::property_id_from_string(fmt::format("{}-left{}", property, post_fix)))
                    == values[1]);
            expect(body.declarations.at(css::property_id_from_string(fmt::format("{}-right{}", property, post_fix)))
                    == values[1]);
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
        return [=]() mutable {
            auto rules = css::parse(fmt::format("p {{ {}: {} {} {}; }}"sv, property, values[0], values[1], values[2]));
            require(rules.size() == 1);

            if (property == "border-style") {
                property = "border";
            }

            auto body = rules[0];
            expect(body.declarations.size() == 4);
            expect(body.declarations.at(css::property_id_from_string(fmt::format("{}-top{}", property, post_fix)))
                    == values[0]);
            expect(body.declarations.at(css::property_id_from_string(fmt::format("{}-bottom{}", property, post_fix)))
                    == values[2]);
            expect(body.declarations.at(css::property_id_from_string(fmt::format("{}-left{}", property, post_fix)))
                    == values[1]);
            expect(body.declarations.at(css::property_id_from_string(fmt::format("{}-right{}", property, post_fix)))
                    == values[1]);
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

    auto box_shorthand_four_values = [](std::string property,
                                             std::array<std::string, 4> values,
                                             std::string post_fix = "") {
        return [=]() mutable {
            auto rules = css::parse(
                    fmt::format("p {{ {}: {} {} {} {}; }}"sv, property, values[0], values[1], values[2], values[3]));
            require(rules.size() == 1);

            if (property == "border-style") {
                property = "border";
            }

            auto body = rules[0];
            expect(body.declarations.size() == 4);
            expect(body.declarations.at(css::property_id_from_string(fmt::format("{}-top{}", property, post_fix)))
                    == values[0]);
            expect(body.declarations.at(css::property_id_from_string(fmt::format("{}-bottom{}", property, post_fix)))
                    == values[2]);
            expect(body.declarations.at(css::property_id_from_string(fmt::format("{}-left{}", property, post_fix)))
                    == values[3]);
            expect(body.declarations.at(css::property_id_from_string(fmt::format("{}-right{}", property, post_fix)))
                    == values[1]);
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

    auto box_shorthand_overridden = [](std::string property,
                                            std::array<std::string, 3> values,
                                            std::string post_fix = "") {
        return [=]() mutable {
            std::string workaround_for_border_style = property == "border-style" ? "border" : property;
            auto rules = css::parse(fmt::format(R"(
                            p {{
                               {0}: {2};
                               {5}-top{1}: {3};
                               {5}-left{1}: {4};
                            }})"sv,
                    property,
                    post_fix,
                    values[0],
                    values[1],
                    values[2],
                    workaround_for_border_style));
            require(rules.size() == 1);

            if (property == "border-style") {
                property = "border";
            }

            auto body = rules[0];
            expect(body.declarations.size() == 4);
            expect_eq(body.declarations.at(css::property_id_from_string(fmt::format("{}-top{}", property, post_fix))),
                    values[1]);
            expect(body.declarations.at(css::property_id_from_string(fmt::format("{}-bottom{}", property, post_fix)))
                    == values[0]);
            expect(body.declarations.at(css::property_id_from_string(fmt::format("{}-left{}", property, post_fix)))
                    == values[2]);
            expect(body.declarations.at(css::property_id_from_string(fmt::format("{}-right{}", property, post_fix)))
                    == values[0]);
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

    auto box_override_with_shorthand = [](std::string property,
                                               std::array<std::string, 4> values,
                                               std::string post_fix = "") {
        return [=]() mutable {
            std::string workaround_for_border_style = property == "border-style" ? "border" : property;
            auto rules = css::parse(fmt::format(R"(
                            p {{
                               {6}-bottom{1}: {2};
                               {6}-left{1}: {3};
                               {0}: {4} {5};
                            }})"sv,
                    property,
                    post_fix,
                    values[0],
                    values[1],
                    values[2],
                    values[3],
                    workaround_for_border_style));
            require(rules.size() == 1);

            if (property == "border-style") {
                property = "border";
            }

            auto body = rules[0];
            expect(body.declarations.size() == 4);
            expect(body.declarations.at(css::property_id_from_string(fmt::format("{}-top{}", property, post_fix)))
                    == values[2]);
            expect(body.declarations.at(css::property_id_from_string(fmt::format("{}-bottom{}", property, post_fix)))
                    == values[2]);
            expect(body.declarations.at(css::property_id_from_string(fmt::format("{}-left{}", property, post_fix)))
                    == values[3]);
            expect(body.declarations.at(css::property_id_from_string(fmt::format("{}-right{}", property, post_fix)))
                    == values[3]);
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

    etest::test("parser: shorthand background color", [] {
        auto rules = css::parse("p { background: red }"sv);
        require(rules.size() == 1);

        auto &p = rules[0];
        expect_eq(get_and_erase(p.declarations, css::PropertyId::BackgroundColor), "red"sv);
        expect(check_initial_background_values(p.declarations));
    });

    etest::test("parser: shorthand font with only size and generic font family", [] {
        auto rules = css::parse("p { font: 1.5em sans-serif; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 20);
        expect(get_and_erase(body.declarations, css::PropertyId::FontFamily) == "sans-serif"s);
        expect(get_and_erase(body.declarations, css::PropertyId::FontSize) == "1.5em"s);
        expect(check_initial_font_values(body.declarations));
    });

    etest::test("parser: shorthand font with size, line height, and generic font family", [] {
        auto rules = css::parse("p { font: 10%/2.5 monospace; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 20);
        expect(get_and_erase(body.declarations, css::PropertyId::FontFamily) == "monospace"s);
        expect(get_and_erase(body.declarations, css::PropertyId::FontSize) == "10%"s);
        expect(get_and_erase(body.declarations, css::PropertyId::LineHeight) == "2.5"s);
        expect(check_initial_font_values(body.declarations));
    });

    etest::test("parser: shorthand font with absolute size, line height, and font family", [] {
        auto rules = css::parse(R"(p { font: x-large/110% "New Century Schoolbook", serif; })"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 20);
        expect(get_and_erase(body.declarations, css::PropertyId::FontFamily) == "\"New Century Schoolbook\", serif"s);
        expect(get_and_erase(body.declarations, css::PropertyId::FontSize) == "x-large"s);
        expect(get_and_erase(body.declarations, css::PropertyId::LineHeight) == "110%"s);
        expect(check_initial_font_values(body.declarations));
    });

    etest::test("parser: shorthand font with italic font style", [] {
        auto rules = css::parse(R"(p { font: italic 120% "Helvetica Neue", serif; })"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 20);
        expect(get_and_erase(body.declarations, css::PropertyId::FontFamily) == "\"Helvetica Neue\", serif"s);
        expect(get_and_erase(body.declarations, css::PropertyId::FontSize) == "120%"s);
        expect(get_and_erase(body.declarations, css::PropertyId::FontStyle) == "italic"s);
        expect(check_initial_font_values(body.declarations));
    });

    etest::test("parser: shorthand font with oblique font style", [] {
        auto rules = css::parse(R"(p { font: oblique 12pt "Helvetica Neue", serif; })"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 20);
        expect(get_and_erase(body.declarations, css::PropertyId::FontFamily) == "\"Helvetica Neue\", serif"s);
        expect(get_and_erase(body.declarations, css::PropertyId::FontSize) == "12pt"s);
        expect(get_and_erase(body.declarations, css::PropertyId::FontStyle) == "oblique"s);
        expect(check_initial_font_values(body.declarations));
    });

    etest::test("parser: shorthand font with font style oblique with angle", [] {
        auto rules = css::parse("p { font: oblique 25deg 10px serif; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 20);
        expect(get_and_erase(body.declarations, css::PropertyId::FontFamily) == "serif"s);
        expect(get_and_erase(body.declarations, css::PropertyId::FontSize) == "10px"s);
        expect(get_and_erase(body.declarations, css::PropertyId::FontStyle) == "oblique 25deg"s);
        expect(check_initial_font_values(body.declarations));
    });

    etest::test("parser: shorthand font with bold font weight", [] {
        auto rules = css::parse("p { font: italic bold 20em/50% serif; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 20);
        expect(get_and_erase(body.declarations, css::PropertyId::FontFamily) == "serif"s);
        expect(get_and_erase(body.declarations, css::PropertyId::FontSize) == "20em"s);
        expect(get_and_erase(body.declarations, css::PropertyId::FontStyle) == "italic"s);
        expect(get_and_erase(body.declarations, css::PropertyId::FontWeight) == "bold"s);
        expect(get_and_erase(body.declarations, css::PropertyId::LineHeight) == "50%"s);
        expect(check_initial_font_values(body.declarations));
    });

    etest::test("parser: shorthand font with bolder font weight", [] {
        auto rules = css::parse("p { font: normal bolder 100px serif; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 20);
        expect(get_and_erase(body.declarations, css::PropertyId::FontFamily) == "serif"s);
        expect(get_and_erase(body.declarations, css::PropertyId::FontSize) == "100px"s);
        expect(get_and_erase(body.declarations, css::PropertyId::FontWeight) == "bolder"s);
        expect(check_initial_font_values(body.declarations));
    });

    etest::test("parser: shorthand font with lighter font weight", [] {
        auto rules = css::parse("p { font: lighter 100px serif; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 20);
        expect(get_and_erase(body.declarations, css::PropertyId::FontFamily) == "serif"s);
        expect(get_and_erase(body.declarations, css::PropertyId::FontSize) == "100px"s);
        expect(get_and_erase(body.declarations, css::PropertyId::FontWeight) == "lighter"s);
        expect(check_initial_font_values(body.declarations));
    });

    etest::test("parser: shorthand font with 1000 font weight", [] {
        auto rules = css::parse("p { font: 1000 oblique 100px serif; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 20);
        expect(get_and_erase(body.declarations, css::PropertyId::FontFamily) == "serif"s);
        expect(get_and_erase(body.declarations, css::PropertyId::FontSize) == "100px"s);
        expect(get_and_erase(body.declarations, css::PropertyId::FontStyle) == "oblique"s);
        expect(get_and_erase(body.declarations, css::PropertyId::FontWeight) == "1000"s);
        expect(check_initial_font_values(body.declarations));
    });

    etest::test("parser: shorthand font with 550 font weight", [] {
        auto rules = css::parse("p { font: italic 550 100px serif; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 20);
        expect(get_and_erase(body.declarations, css::PropertyId::FontFamily) == "serif"s);
        expect(get_and_erase(body.declarations, css::PropertyId::FontSize) == "100px"s);
        expect(get_and_erase(body.declarations, css::PropertyId::FontStyle) == "italic"s);
        expect(get_and_erase(body.declarations, css::PropertyId::FontWeight) == "550"s);
        expect(check_initial_font_values(body.declarations));
    });

    etest::test("parser: shorthand font with 1 font weight", [] {
        auto rules = css::parse("p { font: oblique 1 100px serif; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 20);
        expect(get_and_erase(body.declarations, css::PropertyId::FontFamily) == "serif"s);
        expect(get_and_erase(body.declarations, css::PropertyId::FontSize) == "100px"s);
        expect(get_and_erase(body.declarations, css::PropertyId::FontStyle) == "oblique"s);
        expect(get_and_erase(body.declarations, css::PropertyId::FontWeight) == "1"s);
        expect(check_initial_font_values(body.declarations));
    });

    etest::test("parser: shorthand font with smal1-caps font variant", [] {
        auto rules = css::parse("p { font: small-caps 900 100px serif; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 20);
        expect(get_and_erase(body.declarations, css::PropertyId::FontFamily) == "serif"s);
        expect(get_and_erase(body.declarations, css::PropertyId::FontSize) == "100px"s);
        expect(get_and_erase(body.declarations, css::PropertyId::FontVariant) == "small-caps"s);
        expect(get_and_erase(body.declarations, css::PropertyId::FontWeight) == "900"s);
        expect(check_initial_font_values(body.declarations));
    });

    etest::test("parser: shorthand font with condensed font stretch", [] {
        auto rules = css::parse(R"(p { font: condensed oblique 25deg 753 12pt "Helvetica Neue", serif; })"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 20);
        expect(get_and_erase(body.declarations, css::PropertyId::FontFamily) == "\"Helvetica Neue\", serif"s);
        expect(get_and_erase(body.declarations, css::PropertyId::FontSize) == "12pt"s);
        expect(get_and_erase(body.declarations, css::PropertyId::FontStretch) == "condensed"s);
        expect(get_and_erase(body.declarations, css::PropertyId::FontStyle) == "oblique 25deg"s);
        expect(get_and_erase(body.declarations, css::PropertyId::FontWeight) == "753"s);
        expect(check_initial_font_values(body.declarations));
    });

    etest::test("parser: shorthand font with exapnded font stretch", [] {
        auto rules = css::parse("p { font: italic expanded bold xx-smal/80% monospace; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 20);
        expect(get_and_erase(body.declarations, css::PropertyId::FontFamily) == "monospace"s);
        expect(get_and_erase(body.declarations, css::PropertyId::FontSize) == "xx-smal"s);
        expect(get_and_erase(body.declarations, css::PropertyId::FontStretch) == "expanded"s);
        expect(get_and_erase(body.declarations, css::PropertyId::FontStyle) == "italic"s);
        expect(get_and_erase(body.declarations, css::PropertyId::FontWeight) == "bold"s);
        expect(get_and_erase(body.declarations, css::PropertyId::LineHeight) == "80%"s);
        expect(check_initial_font_values(body.declarations));
    });

    etest::test("parser: font, single-argument", [] {
        auto rules = css::parse("p { font: status-bar; }"sv);
        require(rules.size() == 1);

        auto p = rules[0];
        expect_eq(p.declarations.size(), std::size_t{1});
        expect_eq(get_and_erase(p.declarations, css::PropertyId::FontFamily), "status-bar"s);
    });

    etest::test("parser: shorthand font with ultra-exapnded font stretch", [] {
        auto rules = css::parse("p { font: small-caps italic ultra-expanded bold medium Arial, monospace; }"sv);
        require(rules.size() == 1);

        auto body = rules[0];
        expect(body.declarations.size() == 20);
        expect(get_and_erase(body.declarations, css::PropertyId::FontFamily) == "Arial, monospace"s);
        expect(get_and_erase(body.declarations, css::PropertyId::FontSize) == "medium"s);
        expect(get_and_erase(body.declarations, css::PropertyId::FontStretch) == "ultra-expanded"s);
        expect(get_and_erase(body.declarations, css::PropertyId::FontStyle) == "italic"s);
        expect(get_and_erase(body.declarations, css::PropertyId::FontVariant) == "small-caps"s);
        expect(get_and_erase(body.declarations, css::PropertyId::FontWeight) == "bold"s);
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

    etest::test("parser: several @keyframes in a row doesn't crash the parser", [] {
        auto css = R"(
            @keyframes toast-spinner {
                from { transform: rotate(0deg) }
                to { transform: rotate(360deg) }
            }
            @keyframes toast-spinner {
                from { transform: rotate(0deg) }
                to { transform: rotate(360deg) }
            })"sv;

        // No rules produced (yet!) since this isn't handled aside from not crashing.
        auto rules = css::parse(css);
        expect(rules.empty());
    });

    etest::test("parser: @font-face", [] {
        // This isn't correct, but it doesn't crash.
        auto css = R"(
            @font-face {
                font-family: "Open Sans";
                src: url("/fonts/OpenSans-Regular-webfont.woff2") format("woff2"),
                     url("/fonts/OpenSans-Regular-webfont.woff") format("woff");
            })"sv;

        auto rules = css::parse(css);
        expect_eq(rules.size(), std::size_t{1});
        expect_eq(rules[0].selectors, std::vector{"@font-face"s});
        expect_eq(rules[0].declarations.size(), std::size_t{2});
        expect_eq(rules[0].declarations.at(css::PropertyId::FontFamily), R"("Open Sans")");

        // Very incorrect.
        auto const &src = rules[0].declarations.at(css::PropertyId::Unknown);
        auto woff2 = src.find(R"(url("/fonts/OpenSans-Regular-webfont.woff2") format("woff2"))");
        expect(woff2 != std::string::npos);
        auto woff = src.find(R"(url("/fonts/OpenSans-Regular-webfont.woff") format("woff")");
        expect(woff != std::string::npos);
    });

    return etest::run_all_tests();
}
