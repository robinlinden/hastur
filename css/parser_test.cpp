// SPDX-FileCopyrightText: 2021-2024 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2021 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "css/parser.h"

#include "css/media_query.h"
#include "css/property_id.h"
#include "css/rule.h"

#include "etest/etest2.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <iterator>
#include <map>
#include <source_location>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

using namespace std::literals;

namespace {

// NOLINTNEXTLINE(cert-err58-cpp)
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

// NOLINTNEXTLINE(cert-err58-cpp)
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
ValueT get_and_erase(etest::IActions &a,
        std::map<KeyT, ValueT> &map,
        KeyT key,
        std::source_location const &loc = std::source_location::current()) {
    a.require(map.contains(key), {}, loc);
    ValueT value = map.at(key);
    map.erase(key);
    return value;
}

void text_decoration_tests(etest::Suite &s) {
    s.add_test("parser: text-decoration, line", [](etest::IActions &a) {
        auto rules = css::parse("p { text-decoration: underline; }").rules;
        auto const &p = rules.at(0);
        a.expect_eq(p.declarations,
                std::map<css::PropertyId, std::string>{
                        {css::PropertyId::TextDecorationColor, "currentcolor"},
                        {css::PropertyId::TextDecorationLine, "underline"},
                        {css::PropertyId::TextDecorationStyle, "solid"},
                });
    });

    s.add_test("parser: text-decoration, line & style", [](etest::IActions &a) {
        auto rules = css::parse("p { text-decoration: underline dotted; }").rules;
        auto const &p = rules.at(0);
        a.expect_eq(p.declarations,
                std::map<css::PropertyId, std::string>{
                        {css::PropertyId::TextDecorationColor, "currentcolor"},
                        {css::PropertyId::TextDecorationLine, "underline"},
                        {css::PropertyId::TextDecorationStyle, "dotted"},
                });
    });

    s.add_test("parser: text-decoration, duplicate line", [](etest::IActions &a) {
        auto rules = css::parse("p { text-decoration: underline overline; }").rules;
        auto const &p = rules.at(0);
        a.expect_eq(p.declarations, std::map<css::PropertyId, std::string>{});
    });

    s.add_test("parser: text-decoration, duplicate style", [](etest::IActions &a) {
        auto rules = css::parse("p { text-decoration: dotted dotted; }").rules;
        auto const &p = rules.at(0);
        a.expect_eq(p.declarations, std::map<css::PropertyId, std::string>{});
    });

    // This will fail once we support text-decoration-thickness.
    s.add_test("parser: text-decoration, line & thickness", [](etest::IActions &a) {
        auto rules = css::parse("p { text-decoration: underline 3px; }").rules;
        auto const &p = rules.at(0);
        a.expect_eq(p.declarations, std::map<css::PropertyId, std::string>{});
    });

    // This will fail once we support text-decoration-color.
    s.add_test("parser: text-decoration, line & color", [](etest::IActions &a) {
        auto rules = css::parse("p { text-decoration: overline blue; }").rules;
        auto const &p = rules.at(0);
        a.expect_eq(p.declarations, std::map<css::PropertyId, std::string>{});
    });

    s.add_test("parser: text-decoration, global value", [](etest::IActions &a) {
        auto rules = css::parse("p { text-decoration: inherit; }").rules;
        auto const &p = rules.at(0);
        a.expect_eq(p.declarations,
                std::map<css::PropertyId, std::string>{
                        {css::PropertyId::TextDecorationColor, "inherit"},
                        {css::PropertyId::TextDecorationLine, "inherit"},
                        {css::PropertyId::TextDecorationStyle, "inherit"},
                });
    });
}

void outline_tests(etest::Suite &s) {
    s.add_test("parser: outline shorthand, all values", [](etest::IActions &a) {
        auto rules = css::parse("p { outline: 5px black solid; }").rules;
        auto const &p = rules.at(0);
        a.expect_eq(p.declarations,
                std::map<css::PropertyId, std::string>{
                        {css::PropertyId::OutlineColor, "black"s},
                        {css::PropertyId::OutlineStyle, "solid"s},
                        {css::PropertyId::OutlineWidth, "5px"s},
                });
    });

    s.add_test("parser: outline shorthand, color+style", [](etest::IActions &a) {
        auto rules = css::parse("p { outline: #123 dotted; }").rules;
        auto const &p = rules.at(0);
        a.expect_eq(p.declarations,
                std::map<css::PropertyId, std::string>{
                        {css::PropertyId::OutlineColor, "#123"s},
                        {css::PropertyId::OutlineStyle, "dotted"s},
                        {css::PropertyId::OutlineWidth, "medium"s},
                });
    });

    s.add_test("parser: outline shorthand, width+style", [](etest::IActions &a) {
        auto rules = css::parse("p { outline: ridge 30em; }").rules;
        auto const &p = rules.at(0);
        a.expect_eq(p.declarations,
                std::map<css::PropertyId, std::string>{
                        {css::PropertyId::OutlineColor, "currentcolor"s},
                        {css::PropertyId::OutlineStyle, "ridge"s},
                        {css::PropertyId::OutlineWidth, "30em"s},
                });
    });

    s.add_test("parser: outline shorthand, width", [](etest::IActions &a) {
        auto rules = css::parse("p { outline: thin; }").rules;
        auto const &p = rules.at(0);
        a.expect_eq(p.declarations,
                std::map<css::PropertyId, std::string>{
                        {css::PropertyId::OutlineColor, "currentcolor"s},
                        {css::PropertyId::OutlineStyle, "none"s},
                        {css::PropertyId::OutlineWidth, "thin"s},
                });
    });

    s.add_test("parser: outline shorthand, width, first character a dot", [](etest::IActions &a) {
        auto rules = css::parse("p { outline: .3em; }").rules;
        auto const &p = rules.at(0);
        a.expect_eq(p.declarations,
                std::map<css::PropertyId, std::string>{
                        {css::PropertyId::OutlineColor, "currentcolor"s},
                        {css::PropertyId::OutlineStyle, "none"s},
                        {css::PropertyId::OutlineWidth, ".3em"s},
                });
    });

    s.add_test("parser: outline shorthand, too many values", [](etest::IActions &a) {
        auto rules = css::parse("p { outline: outset #123 none solid; }").rules;
        a.require(rules.size() == 1);
        auto const &p = rules[0];
        a.expect_eq(p.declarations, std::map<css::PropertyId, std::string>{});
    });
}

} // namespace

int main() {
    etest::Suite s{};
    text_decoration_tests(s);
    outline_tests(s);

    s.add_test("parser: simple rule", [](etest::IActions &a) {
        auto rules = css::parse("body { width: 50px; }"sv).rules;
        a.require(rules.size() == 1);

        auto const &body = rules[0];
        a.expect(body.selectors == std::vector{"body"s});
        a.expect(body.declarations.size() == 1);
        a.expect(body.declarations.at(css::PropertyId::Width) == "50px"s);
    });

    s.add_test("parser: important rule", [](etest::IActions &a) {
        auto rules = css::parse("body { width: 50px !important; }"sv).rules;
        a.require_eq(rules.size(), std::size_t{1});

        auto const &body = rules[0];
        a.expect_eq(body.selectors, std::vector{"body"s});
        a.expect(body.declarations.empty());
        a.expect_eq(body.important_declarations.size(), std::size_t{1});
        a.expect_eq(body.important_declarations.at(css::PropertyId::Width), "50px"s);
    });

    s.add_test("selector with spaces", [](etest::IActions &a) {
        auto rules = css::parse("p a { color: green; }").rules;
        a.expect_eq(rules,
                std::vector<css::Rule>{{
                        .selectors{{"p a"}},
                        .declarations{{css::PropertyId::Color, "green"}},
                }});
    });

    s.add_test("property value with spaces", [](etest::IActions &a) {
        auto rules = css::parse("p { color:           green       ; }").rules;
        a.expect_eq(rules,
                std::vector<css::Rule>{{
                        .selectors{{"p"}},
                        .declarations{{css::PropertyId::Color, "green"}},
                }});
    });

    s.add_test("parser: minified", [](etest::IActions &a) {
        auto rules = css::parse("body{width:50px;font-family:inherit}head,p{display:none}"sv).rules;
        a.require(rules.size() == 2);

        auto const &first = rules[0];
        a.expect(first.selectors == std::vector{"body"s});
        a.expect(first.declarations.size() == 2);
        a.expect(first.declarations.at(css::PropertyId::Width) == "50px"s);
        a.expect(first.declarations.at(css::PropertyId::FontFamily) == "inherit"s);

        auto const &second = rules[1];
        a.expect(second.selectors == std::vector{"head"s, "p"s});
        a.expect(second.declarations.size() == 1);
        a.expect(second.declarations.at(css::PropertyId::Display) == "none"s);
    });

    s.add_test("parser: multiple rules", [](etest::IActions &a) {
        auto rules = css::parse("body { width: 50px; }\np { font-size: 8em; }"sv).rules;
        a.require(rules.size() == 2);

        auto const &body = rules[0];
        a.expect(body.selectors == std::vector{"body"s});
        a.expect(body.declarations.size() == 1);
        a.expect(body.declarations.at(css::PropertyId::Width) == "50px"s);

        auto const &p = rules[1];
        a.expect(p.selectors == std::vector{"p"s});
        a.expect(p.declarations.size() == 1);
        a.expect(p.declarations.at(css::PropertyId::FontSize) == "8em"s);
    });

    s.add_test("parser: multiple selectors", [](etest::IActions &a) {
        auto rules = css::parse("body, p { width: 50px; }"sv).rules;
        a.require(rules.size() == 1);

        auto const &body = rules[0];
        a.expect(body.selectors == std::vector{"body"s, "p"s});
        a.expect(body.declarations.size() == 1);
        a.expect(body.declarations.at(css::PropertyId::Width) == "50px"s);
    });

    s.add_test("parser: multiple declarations", [](etest::IActions &a) {
        auto rules = css::parse("body { width: 50px; height: 300px; }"sv).rules;
        a.require(rules.size() == 1);

        auto const &body = rules[0];
        a.expect(body.selectors == std::vector{"body"s});
        a.expect(body.declarations.size() == 2);
        a.expect(body.declarations.at(css::PropertyId::Width) == "50px"s);
        a.expect(body.declarations.at(css::PropertyId::Height) == "300px"s);
    });

    s.add_test("parser: class", [](etest::IActions &a) {
        auto rules = css::parse(".cls { width: 50px; }"sv).rules;
        a.require(rules.size() == 1);

        auto const &body = rules[0];
        a.expect(body.selectors == std::vector{".cls"s});
        a.expect(body.declarations.size() == 1);
        a.expect(body.declarations.at(css::PropertyId::Width) == "50px"s);
    });

    s.add_test("parser: id", [](etest::IActions &a) {
        auto rules = css::parse("#cls { width: 50px; }"sv).rules;
        a.require(rules.size() == 1);

        auto const &body = rules[0];
        a.expect(body.selectors == std::vector{"#cls"s});
        a.expect(body.declarations.size() == 1);
        a.expect(body.declarations.at(css::PropertyId::Width) == "50px"s);
    });

    s.add_test("parser: empty rule", [](etest::IActions &a) {
        auto rules = css::parse("body {}"sv).rules;
        a.require(rules.size() == 1);

        auto const &body = rules[0];
        a.expect(body.selectors == std::vector{"body"s});
        a.expect(body.declarations.empty());
    });

    s.add_test("parser: no rules", [](etest::IActions &a) {
        auto rules = css::parse(""sv).rules;
        a.expect(rules.empty());
    });

    s.add_test("parser: top-level comments", [](etest::IActions &a) {
        auto rules = css::parse("body { width: 50px; }/* comment. */ p { font-size: 8em; } /* comment. */"sv).rules;
        a.require(rules.size() == 2);

        auto const &body = rules[0];
        a.expect(body.selectors == std::vector{"body"s});
        a.expect(body.declarations.size() == 1);
        a.expect(body.declarations.at(css::PropertyId::Width) == "50px"s);

        auto const &p = rules[1];
        a.expect(p.selectors == std::vector{"p"s});
        a.expect(p.declarations.size() == 1);
        a.expect(p.declarations.at(css::PropertyId::FontSize) == "8em"s);
    });

    s.add_test("parser: comments almost everywhere", [](etest::IActions &a) {
        // body { width: 50px; } p { padding: 8em 4em; } with comments added everywhere currently supported.
        auto rules = css::parse(R"(/**/body {/**/width:50px;/**/}/*
                */p {/**/padding:/**/8em 4em;/**//**/}/**/)"sv)
                             .rules;
        // TODO(robinlinden): Support comments in more places.
        // auto rules = css::parse(R"(/**/body/**/{/**/width/**/:/**/50px/**/;/**/}/*
        //         */p/**/{/**/padding/**/:/**/8em/**/4em/**/;/**//**/}/**/)"sv).rules;
        a.require_eq(rules.size(), 2UL);

        auto const &body = rules[0];
        a.expect_eq(body.selectors, std::vector{"body"s});
        a.expect_eq(body.declarations.size(), 1UL);
        a.expect_eq(body.declarations.at(css::PropertyId::Width), "50px"s);

        auto const &p = rules[1];
        a.expect_eq(p.selectors, std::vector{"p"s});
        a.expect_eq(p.declarations.size(), 4UL);
        a.expect_eq(p.declarations.at(css::PropertyId::PaddingTop), "8em"s);
        a.expect_eq(p.declarations.at(css::PropertyId::PaddingBottom), "8em"s);
        a.expect_eq(p.declarations.at(css::PropertyId::PaddingLeft), "4em"s);
        a.expect_eq(p.declarations.at(css::PropertyId::PaddingRight), "4em"s);
    });

    s.add_test("parser: media query", [](etest::IActions &a) {
        auto rules = css::parse(R"(
                @media (min-width: 900px) {
                    article { width: 50px; }
                    p { font-size: 9em; }
                }
                a { background-color: indigo; })"sv)
                             .rules;
        a.require(rules.size() == 3);

        auto const &article = rules[0];
        a.expect(article.selectors == std::vector{"article"s});
        a.require(article.declarations.contains(css::PropertyId::Width));
        a.expect(article.declarations.at(css::PropertyId::Width) == "50px"s);
        a.expect_eq(article.media_query, css::MediaQuery{css::MediaQuery::Width{.min = 900}});

        auto const &p = rules[1];
        a.expect(p.selectors == std::vector{"p"s});
        a.require(p.declarations.contains(css::PropertyId::FontSize));
        a.expect(p.declarations.at(css::PropertyId::FontSize) == "9em"s);
        a.expect_eq(p.media_query, css::MediaQuery{css::MediaQuery::Width{.min = 900}});

        auto const &a_ele = rules[2];
        a.expect(a_ele.selectors == std::vector{"a"s});
        a.require(a_ele.declarations.contains(css::PropertyId::BackgroundColor));
        a.expect(a_ele.declarations.at(css::PropertyId::BackgroundColor) == "indigo"s);
        a.expect(!a_ele.media_query.has_value());
    });

    s.add_test("parser: minified media query", [](etest::IActions &a) {
        auto rules = css::parse("@media(max-width:300px){p{font-size:10px;}}").rules;
        a.require_eq(rules.size(), std::size_t{1});
        auto const &rule = rules[0];
        a.expect_eq(rule.media_query, css::MediaQuery{css::MediaQuery::Width{.max = 300}});
        a.expect_eq(rule.selectors, std::vector{"p"s});
        a.require_eq(rule.declarations.size(), std::size_t{1});
        a.expect_eq(rule.declarations.at(css::PropertyId::FontSize), "10px");
    });

    s.add_test("parser: bad media query", [](etest::IActions &a) {
        auto rules = css::parse("@media (rip: 0) { p { font-size: 10px; } }").rules;
        auto const &rule = rules.at(0);
        a.expect_eq(rule.media_query, css::MediaQuery{css::MediaQuery::False{}});
        a.expect_eq(rule.selectors, std::vector{"p"s});
        a.require_eq(rule.declarations.size(), std::size_t{1});
        a.expect_eq(rule.declarations.at(css::PropertyId::FontSize), "10px");
    });

    s.add_test("parser: 2 media queries in a row", [](etest::IActions &a) {
        auto rules = css::parse(
                "@media (max-width: 1px) { p { font-size: 1em; } } @media (min-width: 2px) { a { color: blue; } }")
                             .rules;
        a.require_eq(rules.size(), std::size_t{2});
        a.expect_eq(rules[0],
                css::Rule{.selectors{{"p"}},
                        .declarations{{css::PropertyId::FontSize, "1em"}},
                        .media_query{css::MediaQuery{css::MediaQuery::Width{.max = 1}}}});
        a.expect_eq(rules[1],
                css::Rule{.selectors{{"a"}},
                        .declarations{{css::PropertyId::Color, "blue"}},
                        .media_query{css::MediaQuery{css::MediaQuery::Width{.min = 2}}}});
    });

    auto box_shorthand_one_value = [](std::string property, std::string value, std::string post_fix = "") {
        return [=](etest::IActions &a) mutable {
            auto rules = css::parse(std::format("p {{ {}: {}; }}"sv, property, value)).rules;
            a.require(rules.size() == 1);

            if (property == "border-style" || property == "border-color" || property == "border-width") {
                property = "border";
            }

            auto const &body = rules[0];
            a.expect(body.declarations.size() == 4);
            a.expect(body.declarations.at(css::property_id_from_string(std::format("{}-top{}", property, post_fix)))
                    == value);
            a.expect(body.declarations.at(css::property_id_from_string(std::format("{}-bottom{}", property, post_fix)))
                    == value);
            a.expect(body.declarations.at(css::property_id_from_string(std::format("{}-left{}", property, post_fix)))
                    == value);
            a.expect(body.declarations.at(css::property_id_from_string(std::format("{}-right{}", property, post_fix)))
                    == value);
        };
    };

    {
        std::string size_value{"10px"};
        s.add_test("parser: shorthand padding, one value", box_shorthand_one_value("padding", size_value));
        s.add_test("parser: shorthand margin, one value", box_shorthand_one_value("margin", size_value));

        std::string border_style{"dashed"};
        s.add_test("parser: shorthand border-style, one value",
                box_shorthand_one_value("border-style", border_style, "-style"));

        s.add_test(
                "parser: shorthand border-color, one value", box_shorthand_one_value("border-color", "red", "-color"));

        s.add_test(
                "parser: shorthand border-width, one value", box_shorthand_one_value("border-width", "10px", "-width"));
    }

    auto box_shorthand_two_values = [](std::string property,
                                            std::array<std::string, 2> values,
                                            std::string post_fix = "") {
        return [=](etest::IActions &a) mutable {
            auto rules = css::parse(std::format("p {{ {}: {} {}; }}"sv, property, values[0], values[1])).rules;
            a.require(rules.size() == 1);

            if (property == "border-style") {
                property = "border";
            }

            auto const &body = rules[0];
            a.expect(body.declarations.size() == 4);
            a.expect(body.declarations.at(css::property_id_from_string(std::format("{}-top{}", property, post_fix)))
                    == values[0]);
            a.expect(body.declarations.at(css::property_id_from_string(std::format("{}-bottom{}", property, post_fix)))
                    == values[0]);
            a.expect(body.declarations.at(css::property_id_from_string(std::format("{}-left{}", property, post_fix)))
                    == values[1]);
            a.expect(body.declarations.at(css::property_id_from_string(std::format("{}-right{}", property, post_fix)))
                    == values[1]);
        };
    };

    {
        auto size_values = std::array{"12em"s, "36em"s};
        s.add_test("parser: shorthand padding, two values", box_shorthand_two_values("padding", size_values));
        s.add_test("parser: shorthand margin, two values", box_shorthand_two_values("margin", size_values));

        auto border_styles = std::array{"dashed"s, "solid"s};
        s.add_test("parser: shorthand border-style, two values",
                box_shorthand_two_values("border-style", border_styles, "-style"));
    }

    auto box_shorthand_three_values = [](std::string property,
                                              std::array<std::string, 3> values,
                                              std::string post_fix = "") {
        return [=](etest::IActions &a) mutable {
            auto rules =
                    css::parse(std::format("p {{ {}: {} {} {}; }}"sv, property, values[0], values[1], values[2])).rules;
            a.require(rules.size() == 1);

            if (property == "border-style") {
                property = "border";
            }

            auto const &body = rules[0];
            a.expect(body.declarations.size() == 4);
            a.expect(body.declarations.at(css::property_id_from_string(std::format("{}-top{}", property, post_fix)))
                    == values[0]);
            a.expect(body.declarations.at(css::property_id_from_string(std::format("{}-bottom{}", property, post_fix)))
                    == values[2]);
            a.expect(body.declarations.at(css::property_id_from_string(std::format("{}-left{}", property, post_fix)))
                    == values[1]);
            a.expect(body.declarations.at(css::property_id_from_string(std::format("{}-right{}", property, post_fix)))
                    == values[1]);
        };
    };

    {
        auto size_values = std::array{"12em"s, "36em"s, "52px"s};
        s.add_test("parser: shorthand padding, three values", box_shorthand_three_values("padding", size_values));
        s.add_test("parser: shorthand margin, three values", box_shorthand_three_values("margin", size_values));

        auto border_styles = std::array{"groove"s, "dashed"s, "solid"s};
        s.add_test("parser: shorthand border-style, three values",
                box_shorthand_three_values("border-style", border_styles, "-style"));
    }

    auto box_shorthand_four_values = [](std::string property,
                                             std::array<std::string, 4> values,
                                             std::string post_fix = "") {
        return [=](etest::IActions &a) mutable {
            auto rules = css::parse(
                    std::format("p {{ {}: {} {} {} {}; }}"sv, property, values[0], values[1], values[2], values[3]))
                                 .rules;
            a.require(rules.size() == 1);

            if (property == "border-style" || property == "border-color" || property == "border-width") {
                property = "border";
            }

            auto const &body = rules[0];
            a.expect(body.declarations.size() == 4);
            a.expect(body.declarations.at(css::property_id_from_string(std::format("{}-top{}", property, post_fix)))
                    == values[0]);
            a.expect(body.declarations.at(css::property_id_from_string(std::format("{}-bottom{}", property, post_fix)))
                    == values[2]);
            a.expect(body.declarations.at(css::property_id_from_string(std::format("{}-left{}", property, post_fix)))
                    == values[3]);
            a.expect(body.declarations.at(css::property_id_from_string(std::format("{}-right{}", property, post_fix)))
                    == values[1]);
        };
    };

    {
        auto size_values = std::array{"12px"s, "36px"s, "52px"s, "2"s};
        s.add_test("parser: shorthand padding, four values", box_shorthand_four_values("padding", size_values));
        s.add_test("parser: shorthand margin, four values", box_shorthand_four_values("margin", size_values));

        auto border_styles = std::array{"groove"s, "dashed"s, "solid"s, "dotted"s};
        s.add_test("parser: shorthand border-style, four values",
                box_shorthand_four_values("border-style", border_styles, "-style"));

        s.add_test("parser: shorthand border-color, four values",
                box_shorthand_four_values("border-color", std::array{"red"s, "green"s, "blue"s, "cyan"s}, "-color"));

        s.add_test("parser: shorthand border-width, four values",
                box_shorthand_four_values("border-width", size_values, "-width"));
    }

    auto box_shorthand_overridden = [](std::string property,
                                            std::array<std::string, 3> values,
                                            std::string post_fix = "") {
        return [=](etest::IActions &a) mutable {
            std::string workaround_for_border_style = property == "border-style" ? "border" : property;
            auto rules = css::parse(std::format(R"(
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
                                            workaround_for_border_style))
                                 .rules;
            a.require(rules.size() == 1);

            if (property == "border-style") {
                property = "border";
            }

            auto const &body = rules[0];
            a.expect(body.declarations.size() == 4);
            a.expect_eq(body.declarations.at(css::property_id_from_string(std::format("{}-top{}", property, post_fix))),
                    values[1]);
            a.expect(body.declarations.at(css::property_id_from_string(std::format("{}-bottom{}", property, post_fix)))
                    == values[0]);
            a.expect(body.declarations.at(css::property_id_from_string(std::format("{}-left{}", property, post_fix)))
                    == values[2]);
            a.expect(body.declarations.at(css::property_id_from_string(std::format("{}-right{}", property, post_fix)))
                    == values[0]);
        };
    };

    {
        auto size_values = std::array{"10px"s, "15px"s, "25px"s};
        s.add_test("parser: shorthand padding overridden", box_shorthand_overridden("padding", size_values));
        s.add_test("parser: shorthand margin overridden", box_shorthand_overridden("margin", size_values));

        auto border_styles = std::array{"dashed"s, "solid"s, "dotted"s};
        s.add_test("parser: shorthand border-style overridden",
                box_shorthand_overridden("border-style", border_styles, "-style"));
    }

    auto box_override_with_shorthand = [](std::string property,
                                               std::array<std::string, 4> values,
                                               std::string post_fix = "") {
        return [=](etest::IActions &a) mutable {
            std::string workaround_for_border_style = property == "border-style" ? "border" : property;
            auto rules = css::parse(std::format(R"(
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
                                            workaround_for_border_style))
                                 .rules;
            a.require(rules.size() == 1);

            if (property == "border-style") {
                property = "border";
            }

            auto const &body = rules[0];
            a.expect(body.declarations.size() == 4);
            a.expect(body.declarations.at(css::property_id_from_string(std::format("{}-top{}", property, post_fix)))
                    == values[2]);
            a.expect(body.declarations.at(css::property_id_from_string(std::format("{}-bottom{}", property, post_fix)))
                    == values[2]);
            a.expect(body.declarations.at(css::property_id_from_string(std::format("{}-left{}", property, post_fix)))
                    == values[3]);
            a.expect(body.declarations.at(css::property_id_from_string(std::format("{}-right{}", property, post_fix)))
                    == values[3]);
        };
    };

    {
        auto size_values = std::array{"5px"s, "25px"s, "12px"s, "40px"s};
        s.add_test("parser: override padding with shorthand", box_override_with_shorthand("padding", size_values));
        s.add_test("parser: override margin with shorthand", box_override_with_shorthand("margin", size_values));

        auto border_styles = std::array{"dashed"s, "solid"s, "hidden"s, "dotted"s};
        s.add_test("parser: override border-style with shorthand",
                box_override_with_shorthand("border-style", border_styles, "-style"));
    }

    s.add_test("parser: shorthand background color", [](etest::IActions &a) {
        auto rules = css::parse("p { background: red }"sv).rules;
        a.require(rules.size() == 1);

        auto &p = rules[0];
        a.expect_eq(get_and_erase(a, p.declarations, css::PropertyId::BackgroundColor), "red"sv);
        a.expect(check_initial_background_values(p.declarations));
    });

    s.add_test("parser: shorthand font with only size and generic font family", [](etest::IActions &a) {
        auto rules = css::parse("p { font: 1.5em sans-serif; }"sv).rules;
        a.require(rules.size() == 1);

        auto body = rules[0];
        a.expect(body.declarations.size() == 20);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontFamily) == "sans-serif"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontSize) == "1.5em"s);
        a.expect(check_initial_font_values(body.declarations));
    });

    s.add_test("parser: shorthand font with size, line height, and generic font family", [](etest::IActions &a) {
        auto rules = css::parse("p { font: 10%/2.5 monospace; }"sv).rules;
        a.require(rules.size() == 1);

        auto body = rules[0];
        a.expect(body.declarations.size() == 20);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontFamily) == "monospace"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontSize) == "10%"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::LineHeight) == "2.5"s);
        a.expect(check_initial_font_values(body.declarations));
    });

    s.add_test("parser: shorthand font with absolute size, line height, and font family", [](etest::IActions &a) {
        auto rules = css::parse(R"(p { font: x-large/110% "New Century Schoolbook", serif; })"sv).rules;
        a.require(rules.size() == 1);

        auto body = rules[0];
        a.expect(body.declarations.size() == 20);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontFamily)
                == R"("New Century Schoolbook", serif)"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontSize) == "x-large"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::LineHeight) == "110%"s);
        a.expect(check_initial_font_values(body.declarations));
    });

    s.add_test("parser: shorthand font with italic font style", [](etest::IActions &a) {
        auto rules = css::parse(R"(p { font: italic 120% "Helvetica Neue", serif; })"sv).rules;
        a.require(rules.size() == 1);

        auto body = rules[0];
        a.expect(body.declarations.size() == 20);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontFamily) == R"("Helvetica Neue", serif)"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontSize) == "120%"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontStyle) == "italic"s);
        a.expect(check_initial_font_values(body.declarations));
    });

    s.add_test("parser: shorthand font with oblique font style", [](etest::IActions &a) {
        auto rules = css::parse(R"(p { font: oblique 12pt "Helvetica Neue", serif; })"sv).rules;
        a.require(rules.size() == 1);

        auto body = rules[0];
        a.expect(body.declarations.size() == 20);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontFamily) == R"("Helvetica Neue", serif)"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontSize) == "12pt"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontStyle) == "oblique"s);
        a.expect(check_initial_font_values(body.declarations));
    });

    s.add_test("parser: shorthand font with font style oblique with angle", [](etest::IActions &a) {
        auto rules = css::parse("p { font: oblique 25deg 10px serif; }"sv).rules;
        a.require(rules.size() == 1);

        auto body = rules[0];
        a.expect(body.declarations.size() == 20);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontFamily) == "serif"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontSize) == "10px"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontStyle) == "oblique 25deg"s);
        a.expect(check_initial_font_values(body.declarations));
    });

    s.add_test("parser: shorthand font with bold font weight", [](etest::IActions &a) {
        auto rules = css::parse("p { font: italic bold 20em/50% serif; }"sv).rules;
        a.require(rules.size() == 1);

        auto body = rules[0];
        a.expect(body.declarations.size() == 20);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontFamily) == "serif"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontSize) == "20em"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontStyle) == "italic"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontWeight) == "bold"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::LineHeight) == "50%"s);
        a.expect(check_initial_font_values(body.declarations));
    });

    s.add_test("parser: shorthand font with bolder font weight", [](etest::IActions &a) {
        auto rules = css::parse("p { font: normal bolder 100px serif; }"sv).rules;
        a.require(rules.size() == 1);

        auto body = rules[0];
        a.expect(body.declarations.size() == 20);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontFamily) == "serif"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontSize) == "100px"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontWeight) == "bolder"s);
        a.expect(check_initial_font_values(body.declarations));
    });

    s.add_test("parser: shorthand font with lighter font weight", [](etest::IActions &a) {
        auto rules = css::parse("p { font: lighter 100px serif; }"sv).rules;
        a.require(rules.size() == 1);

        auto body = rules[0];
        a.expect(body.declarations.size() == 20);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontFamily) == "serif"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontSize) == "100px"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontWeight) == "lighter"s);
        a.expect(check_initial_font_values(body.declarations));
    });

    s.add_test("parser: shorthand font with 1000 font weight", [](etest::IActions &a) {
        auto rules = css::parse("p { font: 1000 oblique 100px serif; }"sv).rules;
        a.require(rules.size() == 1);

        auto body = rules[0];
        a.expect(body.declarations.size() == 20);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontFamily) == "serif"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontSize) == "100px"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontStyle) == "oblique"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontWeight) == "1000"s);
        a.expect(check_initial_font_values(body.declarations));
    });

    s.add_test("parser: shorthand font with 550 font weight", [](etest::IActions &a) {
        auto rules = css::parse("p { font: italic 550 100px serif; }"sv).rules;
        a.require(rules.size() == 1);

        auto body = rules[0];
        a.expect(body.declarations.size() == 20);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontFamily) == "serif"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontSize) == "100px"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontStyle) == "italic"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontWeight) == "550"s);
        a.expect(check_initial_font_values(body.declarations));
    });

    s.add_test("parser: shorthand font with 1 font weight", [](etest::IActions &a) {
        auto rules = css::parse("p { font: oblique 1 100px serif; }"sv).rules;
        a.require(rules.size() == 1);

        auto body = rules[0];
        a.expect(body.declarations.size() == 20);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontFamily) == "serif"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontSize) == "100px"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontStyle) == "oblique"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontWeight) == "1"s);
        a.expect(check_initial_font_values(body.declarations));
    });

    s.add_test("parser: shorthand font with smal1-caps font variant", [](etest::IActions &a) {
        auto rules = css::parse("p { font: small-caps 900 100px serif; }"sv).rules;
        a.require(rules.size() == 1);

        auto body = rules[0];
        a.expect(body.declarations.size() == 20);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontFamily) == "serif"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontSize) == "100px"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontVariant) == "small-caps"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontWeight) == "900"s);
        a.expect(check_initial_font_values(body.declarations));
    });

    s.add_test("parser: shorthand font with condensed font stretch", [](etest::IActions &a) {
        auto rules = css::parse(R"(p { font: condensed oblique 25deg 753 12pt "Helvetica Neue", serif; })"sv).rules;
        a.require(rules.size() == 1);

        auto body = rules[0];
        a.expect(body.declarations.size() == 20);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontFamily) == R"("Helvetica Neue", serif)"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontSize) == "12pt"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontStretch) == "condensed"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontStyle) == "oblique 25deg"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontWeight) == "753"s);
        a.expect(check_initial_font_values(body.declarations));
    });

    s.add_test("parser: shorthand font with exapnded font stretch", [](etest::IActions &a) {
        auto rules = css::parse("p { font: italic expanded bold xx-smal/80% monospace; }"sv).rules;
        a.require(rules.size() == 1);

        auto body = rules[0];
        a.expect(body.declarations.size() == 20);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontFamily) == "monospace"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontSize) == "xx-smal"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontStretch) == "expanded"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontStyle) == "italic"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontWeight) == "bold"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::LineHeight) == "80%"s);
        a.expect(check_initial_font_values(body.declarations));
    });

    s.add_test("parser: font, single-argument", [](etest::IActions &a) {
        auto rules = css::parse("p { font: status-bar; }"sv).rules;
        a.require(rules.size() == 1);

        auto p = rules[0];
        a.expect_eq(p.declarations.size(), std::size_t{1});
        a.expect_eq(get_and_erase(a, p.declarations, css::PropertyId::FontFamily), "status-bar"s);
    });

    s.add_test("parser: shorthand font with ultra-exapnded font stretch", [](etest::IActions &a) {
        auto rules = css::parse("p { font: small-caps italic ultra-expanded bold medium Arial, monospace; }"sv).rules;
        a.require(rules.size() == 1);

        auto body = rules[0];
        a.expect(body.declarations.size() == 20);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontFamily) == "Arial, monospace"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontSize) == "medium"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontStretch) == "ultra-expanded"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontStyle) == "italic"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontVariant) == "small-caps"s);
        a.expect(get_and_erase(a, body.declarations, css::PropertyId::FontWeight) == "bold"s);
        a.expect(check_initial_font_values(body.declarations));
    });

    s.add_test("parser: border-radius shorthand, 1 value", [](etest::IActions &a) {
        auto rules = css::parse("div { border-radius: 5px; }").rules;
        a.require(rules.size() == 1);
        auto const &div = rules[0];
        a.expect_eq(div.declarations,
                std::map<css::PropertyId, std::string>{
                        {css::PropertyId::BorderTopLeftRadius, "5px"s},
                        {css::PropertyId::BorderTopRightRadius, "5px"s},
                        {css::PropertyId::BorderBottomRightRadius, "5px"s},
                        {css::PropertyId::BorderBottomLeftRadius, "5px"s},
                });
    });

    s.add_test("parser: border-radius shorthand, 2 values", [](etest::IActions &a) {
        auto rules = css::parse("div { border-radius: 1px 2px; }").rules;
        a.require(rules.size() == 1);
        auto const &div = rules[0];
        a.expect_eq(div.declarations,
                std::map<css::PropertyId, std::string>{
                        {css::PropertyId::BorderTopLeftRadius, "1px"s},
                        {css::PropertyId::BorderTopRightRadius, "2px"s},
                        {css::PropertyId::BorderBottomRightRadius, "1px"s},
                        {css::PropertyId::BorderBottomLeftRadius, "2px"s},
                });
    });

    s.add_test("parser: border-radius shorthand, 3 values", [](etest::IActions &a) {
        auto rules = css::parse("div { border-radius: 1px 2px 3px; }").rules;
        a.require(rules.size() == 1);
        auto const &div = rules[0];
        a.expect_eq(div.declarations,
                std::map<css::PropertyId, std::string>{
                        {css::PropertyId::BorderTopLeftRadius, "1px"s},
                        {css::PropertyId::BorderTopRightRadius, "2px"s},
                        {css::PropertyId::BorderBottomRightRadius, "3px"s},
                        {css::PropertyId::BorderBottomLeftRadius, "2px"s},
                });
    });

    s.add_test("parser: border-radius shorthand, 4 values", [](etest::IActions &a) {
        auto rules = css::parse("div { border-radius: 1px 2px 3px 4px; }").rules;
        a.require(rules.size() == 1);
        auto const &div = rules[0];
        a.expect_eq(div.declarations,
                std::map<css::PropertyId, std::string>{
                        {css::PropertyId::BorderTopLeftRadius, "1px"s},
                        {css::PropertyId::BorderTopRightRadius, "2px"s},
                        {css::PropertyId::BorderBottomRightRadius, "3px"s},
                        {css::PropertyId::BorderBottomLeftRadius, "4px"s},
                });
    });

    s.add_test("parser: border-radius, 1 value, separate horizontal and vertical", [](etest::IActions &a) {
        auto rules = css::parse("div { border-radius: 5px / 10px; }").rules;
        a.require(rules.size() == 1);
        auto const &div = rules[0];
        a.expect_eq(div.declarations,
                std::map<css::PropertyId, std::string>{
                        {css::PropertyId::BorderTopLeftRadius, "5px / 10px"s},
                        {css::PropertyId::BorderTopRightRadius, "5px / 10px"s},
                        {css::PropertyId::BorderBottomRightRadius, "5px / 10px"s},
                        {css::PropertyId::BorderBottomLeftRadius, "5px / 10px"s},
                });
    });

    s.add_test("parser: border-radius, 2 values, separate horizontal and vertical", [](etest::IActions &a) {
        auto rules = css::parse("div { border-radius: 5px / 10px 15px; }").rules;
        a.require(rules.size() == 1);
        auto const &div = rules[0];
        a.expect_eq(div.declarations,
                std::map<css::PropertyId, std::string>{
                        {css::PropertyId::BorderTopLeftRadius, "5px / 10px"s},
                        {css::PropertyId::BorderTopRightRadius, "5px / 15px"s},
                        {css::PropertyId::BorderBottomRightRadius, "5px / 10px"s},
                        {css::PropertyId::BorderBottomLeftRadius, "5px / 15px"s},
                });
    });

    s.add_test("parser: border-radius, 3 values, separate horizontal and vertical", [](etest::IActions &a) {
        auto rules = css::parse("div { border-radius: 5px / 10px 15px 20px; }").rules;
        a.require(rules.size() == 1);
        auto const &div = rules[0];
        a.expect_eq(div.declarations,
                std::map<css::PropertyId, std::string>{
                        {css::PropertyId::BorderTopLeftRadius, "5px / 10px"s},
                        {css::PropertyId::BorderTopRightRadius, "5px / 15px"s},
                        {css::PropertyId::BorderBottomRightRadius, "5px / 20px"s},
                        {css::PropertyId::BorderBottomLeftRadius, "5px / 15px"s},
                });
    });

    s.add_test("parser: border-radius, 4 values, separate horizontal and vertical", [](etest::IActions &a) {
        auto rules = css::parse("div { border-radius: 5px / 10px 15px 20px 25px; }").rules;
        a.require(rules.size() == 1);
        auto const &div = rules[0];
        a.expect_eq(div.declarations,
                std::map<css::PropertyId, std::string>{
                        {css::PropertyId::BorderTopLeftRadius, "5px / 10px"s},
                        {css::PropertyId::BorderTopRightRadius, "5px / 15px"s},
                        {css::PropertyId::BorderBottomRightRadius, "5px / 20px"s},
                        {css::PropertyId::BorderBottomLeftRadius, "5px / 25px"s},
                });
    });

    s.add_test("parser: border-radius, invalid vertical, separate horizontal and vertical", [](etest::IActions &a) {
        auto rules = css::parse("div { border-radius: 5px / 10px 15px 20px 25px 30px; }").rules;
        a.require(rules.size() == 1);
        auto const &div = rules[0];
        a.expect_eq(div.declarations,
                std::map<css::PropertyId, std::string>{
                        {css::PropertyId::BorderTopLeftRadius, "5px"s},
                        {css::PropertyId::BorderTopRightRadius, "5px"s},
                        {css::PropertyId::BorderBottomRightRadius, "5px"s},
                        {css::PropertyId::BorderBottomLeftRadius, "5px"s},
                });
    });

    s.add_test("parser: @keyframes doesn't crash the parser", [](etest::IActions &a) {
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
        auto rules = css::parse(css).rules;
        a.expect(rules.empty());
    });

    s.add_test("parser: several @keyframes in a row doesn't crash the parser", [](etest::IActions &a) {
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
        auto rules = css::parse(css).rules;
        a.expect(rules.empty());
    });

    s.add_test("parser: @font-face", [](etest::IActions &a) {
        // This isn't correct, but it doesn't crash.
        auto css = R"(
            @font-face {
                font-family: "Open Sans";
                src: url("/fonts/OpenSans-Regular-webfont.woff2") format("woff2"),
                     url("/fonts/OpenSans-Regular-webfont.woff") format("woff");
            })"sv;

        auto rules = css::parse(css).rules;
        a.expect_eq(rules.size(), std::size_t{1});
        a.expect_eq(rules[0].selectors, std::vector{"@font-face"s});
        a.expect_eq(rules[0].declarations.size(), std::size_t{2});
        a.expect_eq(rules[0].declarations.at(css::PropertyId::FontFamily), R"("Open Sans")");

        // Very incorrect.
        auto const &src = rules[0].declarations.at(css::PropertyId::Unknown);
        a.expect(src.contains(R"(url("/fonts/OpenSans-Regular-webfont.woff2") format("woff2"))"));
        a.expect(src.contains(R"(url("/fonts/OpenSans-Regular-webfont.woff") format("woff")"));
    });

    s.add_test("parser: border shorthand, all values", [](etest::IActions &a) {
        auto rules = css::parse("p { border: 5px black solid; }").rules;
        a.require(rules.size() == 1);
        auto const &p = rules[0];
        a.expect_eq(p.declarations,
                std::map<css::PropertyId, std::string>{
                        {css::PropertyId::BorderBottomColor, "black"s},
                        {css::PropertyId::BorderBottomStyle, "solid"s},
                        {css::PropertyId::BorderBottomWidth, "5px"s},
                        {css::PropertyId::BorderLeftColor, "black"s},
                        {css::PropertyId::BorderLeftStyle, "solid"s},
                        {css::PropertyId::BorderLeftWidth, "5px"s},
                        {css::PropertyId::BorderRightColor, "black"s},
                        {css::PropertyId::BorderRightStyle, "solid"s},
                        {css::PropertyId::BorderRightWidth, "5px"s},
                        {css::PropertyId::BorderTopColor, "black"s},
                        {css::PropertyId::BorderTopStyle, "solid"s},
                        {css::PropertyId::BorderTopWidth, "5px"s},
                });
    });

    s.add_test("parser: border shorthand, color+style", [](etest::IActions &a) {
        auto rules = css::parse("p { border-bottom: #123 dotted; }").rules;
        a.require(rules.size() == 1);
        auto const &p = rules[0];
        a.expect_eq(p.declarations,
                std::map<css::PropertyId, std::string>{
                        {css::PropertyId::BorderBottomColor, "#123"s},
                        {css::PropertyId::BorderBottomStyle, "dotted"s},
                        {css::PropertyId::BorderBottomWidth, "medium"s},
                });
    });

    s.add_test("parser: border shorthand, width+style", [](etest::IActions &a) {
        auto rules = css::parse("p { border-left: ridge 30em; }").rules;
        a.require(rules.size() == 1);
        auto const &p = rules[0];
        a.expect_eq(p.declarations,
                std::map<css::PropertyId, std::string>{
                        {css::PropertyId::BorderLeftColor, "currentcolor"s},
                        {css::PropertyId::BorderLeftStyle, "ridge"s},
                        {css::PropertyId::BorderLeftWidth, "30em"s},
                });
    });

    s.add_test("parser: border shorthand, width", [](etest::IActions &a) {
        auto rules = css::parse("p { border-right: thin; }").rules;
        a.require(rules.size() == 1);
        auto const &p = rules[0];
        a.expect_eq(p.declarations,
                std::map<css::PropertyId, std::string>{
                        {css::PropertyId::BorderRightColor, "currentcolor"s},
                        {css::PropertyId::BorderRightStyle, "none"s},
                        {css::PropertyId::BorderRightWidth, "thin"s},
                });
    });

    s.add_test("parser: border shorthand, width, first character a dot", [](etest::IActions &a) {
        auto rules = css::parse("p { border-right: .3em; }").rules;
        a.require(rules.size() == 1);
        auto const &p = rules[0];
        a.expect_eq(p.declarations,
                std::map<css::PropertyId, std::string>{
                        {css::PropertyId::BorderRightColor, "currentcolor"s},
                        {css::PropertyId::BorderRightStyle, "none"s},
                        {css::PropertyId::BorderRightWidth, ".3em"s},
                });
    });

    s.add_test("parser: border shorthand, too many values", [](etest::IActions &a) {
        auto rules = css::parse("p { border-top: outset #123 none solid; }").rules;
        a.require(rules.size() == 1);
        auto const &p = rules[0];
        a.expect_eq(p.declarations, std::map<css::PropertyId, std::string>{});
    });

    s.add_test("parser: incomplete media-query crash", [](etest::IActions &) {
        std::ignore = css::parse("@media("); //
    });

    s.add_test("parser: incomplete at-rule crash", [](etest::IActions &) {
        std::ignore = css::parse("@lol"); //
    });

    s.add_test("parser: incomplete rule in unknown at-rule crash", [](etest::IActions &) {
        std::ignore = css::parse("@lol ");
        std::ignore = css::parse("@lol { p {"); //
    });

    s.add_test("parser: incomplete rule crash", [](etest::IActions &) {
        std::ignore = css::parse("p");
        std::ignore = css::parse("p {");
        std::ignore = css::parse("p { font-size:");
    });

    s.add_test("parser: flex-flow shorthand, global value", [](etest::IActions &a) {
        a.expect_eq(css::parse("p { flex-flow: revert; }").rules.at(0).declarations,
                std::map<css::PropertyId, std::string>{
                        {css::PropertyId::FlexDirection, "revert"s},
                        {css::PropertyId::FlexWrap, "revert"s},
                });
        a.expect_eq(css::parse("p { flex-flow: revert row; }").rules.at(0).declarations,
                std::map<css::PropertyId, std::string>{});
    });

    s.add_test("parser: flex-flow shorthand, one value", [](etest::IActions &a) {
        a.expect_eq(css::parse("p { flex-flow: column; }").rules.at(0).declarations,
                std::map<css::PropertyId, std::string>{
                        {css::PropertyId::FlexDirection, "column"s},
                        {css::PropertyId::FlexWrap, "nowrap"s},
                });
        a.expect_eq(css::parse("p { flex-flow: wrap; }").rules.at(0).declarations,
                std::map<css::PropertyId, std::string>{
                        {css::PropertyId::FlexDirection, "row"s},
                        {css::PropertyId::FlexWrap, "wrap"s},
                });
        a.expect_eq(css::parse("p { flex-flow: aaaaaaaaaa; }").rules.at(0).declarations,
                std::map<css::PropertyId, std::string>{});
    });

    s.add_test("parser: flex-flow shorthand, two values", [](etest::IActions &a) {
        a.expect_eq(css::parse("p { flex-flow: column wrap; }").rules.at(0).declarations,
                std::map<css::PropertyId, std::string>{
                        {css::PropertyId::FlexDirection, "column"s},
                        {css::PropertyId::FlexWrap, "wrap"s},
                });
        a.expect_eq(css::parse("p { flex-flow: wrap wrap; }").rules.at(0).declarations, //
                std::map<css::PropertyId, std::string>{});
        a.expect_eq(css::parse("p { flex-flow: wrap asdf; }").rules.at(0).declarations, //
                std::map<css::PropertyId, std::string>{});
    });

    s.add_test("parser: flex-flow shorthand, too many values :(", [](etest::IActions &a) {
        a.expect_eq(css::parse("p { flex-flow: column wrap nowrap; }").rules.at(0).declarations,
                std::map<css::PropertyId, std::string>{});
    });

    s.add_test("parser: custom property", [](etest::IActions &a) {
        a.expect_eq(css::parse("p { --var: value; }").rules.at(0), //
                css::Rule{.selectors = {{"p"}}, .custom_properties = {{"--var", "value"}}});
    });

    // TODO(robinlinden): Nested rules are currently skipped, but at least
    // they mostly don't break parsing of the rule they're nested in.
    s.add_test("parser: nested rule", [](etest::IActions &a) {
        a.expect_eq(css::parse("p { color: green; a { font-size: 3px; } font-size: 5px; }").rules, //
                std::vector{
                        css::Rule{
                                .selectors = {{"p"}},
                                .declarations{
                                        {css::PropertyId::Color, "green"},
                                        {css::PropertyId::FontSize, "5px"},
                                },
                        },
                });
    });

    s.add_test("parser: eof in nested rule", [](etest::IActions &a) {
        a.expect(css::parse("p { color: green; a { font-size: 3px; ").rules.empty()); //
    });

    s.add_test("parser: -webkit-lol", [](etest::IActions &a) {
        a.expect(css::parse("p { -webkit-font-size: 3px; }").rules.at(0).declarations.empty()); //
    });

    s.add_test("parser: @charset", [](etest::IActions &a) {
        a.expect_eq(css::parse("@charset 'shift-jis'; p { font-size: 3px; }").rules.at(0),
                css::Rule{.selectors = {{"p"}}, .declarations{{css::PropertyId::FontSize, "3px"}}});
    });

    s.add_test("parser: @charset eof", [](etest::IActions &a) {
        a.expect(css::parse("@charset 'shi").rules.empty()); //
    });

    s.add_test("parser: @import", [](etest::IActions &a) {
        a.expect_eq(css::parse("@import 'test.css'; p { font-size: 3px; }").rules.at(0),
                css::Rule{{"p"}, {{css::PropertyId::FontSize, "3px"}}});
    });

    s.add_test("parser: IE hacks don't break things", [](etest::IActions &a) {
        auto rules = css::parse("p { font-size: 3px; *font-size: 5px; } a { color: green; }").rules;
        a.expect_eq(rules,
                std::vector{
                        css::Rule{{"p"}, {{css::PropertyId::FontSize, "3px"}}},
                        css::Rule{{"a"}, {{css::PropertyId::Color, "green"}}},
                });
    });

    return s.run();
}
