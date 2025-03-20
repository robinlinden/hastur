// SPDX-FileCopyrightText: 2021-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "style/styled_node.h"

#include "style/unresolved_value.h"

#include "css/property_id.h"
#include "dom/dom.h"
#include "dom/xpath.h"
#include "etest/etest2.h"
#include "gfx/color.h"

#include <optional>
#include <source_location>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

using namespace std::literals;

namespace {
template<css::PropertyId IdT>
void expect_property_eq(etest::IActions &a,
        std::optional<std::string> value,
        auto const &expected,
        std::vector<std::pair<css::PropertyId, std::string>> extra_properties = {},
        std::source_location const &loc = std::source_location::current()) {
    if (value) {
        extra_properties.emplace_back(IdT, std::move(*value));
    }

    dom::Node dom_node = dom::Element{"dummy"s};
    style::StyledNode styled_node{
            .node = dom_node,
            .properties = std::move(extra_properties),
            .children = {},
    };

    a.expect_eq(styled_node.get_property<IdT>(), expected, std::nullopt, loc);
};

template<css::PropertyId IdT>
void expect_relative_property_eq(etest::IActions &a,
        std::string value,
        std::string parent_value,
        auto expected,
        std::source_location const &loc = std::source_location::current()) {
    dom::Node dom_node = dom::Element{"dummy"s};
    style::StyledNode styled_node{
            .node = dom_node,
            .properties = {{IdT, std::move(parent_value)}},
            .children = {{dom_node, {{IdT, std::move(value)}}, {}}},
    };
    styled_node.children.at(0).parent = &styled_node;

    a.expect_eq(styled_node.children.at(0).get_property<IdT>(), expected, std::nullopt, loc);
};
} // namespace

int main() {
    using enum css::PropertyId;
    etest::Suite s;

    s.add_test("get_property", [](etest::IActions &a) {
        expect_property_eq<css::PropertyId::Width>(a, "15px", style::UnresolvedValue{"15px"}); //
    });

    s.add_test("property inheritance", [](etest::IActions &a) {
        dom::Node dom_node = dom::Element{"dummy"s};
        style::StyledNode root{
                .node = dom_node,
                .properties = {{css::PropertyId::FontSize, "15px"s}, {css::PropertyId::Width, "0px"s}},
                .children = {},
        };

        auto &child = root.children.emplace_back(style::StyledNode{dom_node, {}, {}, &root});

        // Not inherited, returns the initial value.
        a.expect_eq(child.get_property<css::PropertyId::Width>(), style::UnresolvedValue{"auto"});

        // Inherited, returns the parent's value.
        a.expect_eq(child.get_property<css::PropertyId::FontSize>(), 15);
    });

    s.add_test("initial css keyword", [](etest::IActions &a) {
        dom::Node dom_node = dom::Element{"dummy"s};
        style::StyledNode root{
                .node = dom_node,
                .properties = {{css::PropertyId::Color, "blue"s}},
                .children{
                        style::StyledNode{
                                .node{dom_node},
                                .properties{{css::PropertyId::Color, "initial"s}},
                        },
                },
        };

        auto &child = root.children[0];
        child.parent = &root;

        a.expect_eq(child.get_property<css::PropertyId::Color>(), gfx::Color::from_css_name("canvastext"));
    });

    s.add_test("inherit css keyword", [](etest::IActions &a) {
        dom::Node dom_node = dom::Element{"dummy"s};
        style::StyledNode root{
                .node = dom_node,
                .properties = {{css::PropertyId::BackgroundColor, "blue"s}},
                .children{
                        style::StyledNode{
                                .node{dom_node},
                                .properties{
                                        {css::PropertyId::BackgroundColor, "inherit"s},
                                        {css::PropertyId::Width, "inherit"s},
                                },
                        },
                },
        };

        auto &child = root.children[0];
        child.parent = &root;

        // inherit, but not in parent, so receives initial value for property.
        a.expect_eq(child.get_property<css::PropertyId::Width>(), style::UnresolvedValue{"auto"});

        // inherit, value in parent.
        a.expect_eq(child.get_property<css::PropertyId::BackgroundColor>(), gfx::Color::from_css_name("blue"));

        // inherit, no parent node.
        child.parent = nullptr;
        a.expect_eq(child.get_property<css::PropertyId::BackgroundColor>(), gfx::Color::from_css_name("transparent"));
    });

    s.add_test("unset css keyword", [](etest::IActions &a) {
        dom::Node dom_node = dom::Element{"dummy"s};
        style::StyledNode root{
                .node = dom_node,
                .properties = {{css::PropertyId::Color, "blue"s}},
                .children{
                        style::StyledNode{
                                .node{dom_node},
                                .properties{
                                        {css::PropertyId::Color, "unset"s},
                                        {css::PropertyId::Width, "unset"s},
                                },
                        },
                },
        };

        auto &child = root.children[0];
        child.parent = &root;

        // unset, not inherited, so receives initial value for property.
        a.expect_eq(child.get_property<css::PropertyId::Width>(), style::UnresolvedValue{"auto"});

        // unset, inherited, value in parent.
        a.expect_eq(child.get_property<css::PropertyId::Color>(), gfx::Color::from_css_name("blue"));

        // unset, inherited, no parent node.
        child.parent = nullptr;
        a.expect_eq(child.get_property<css::PropertyId::Color>(), gfx::Color::from_css_name("canvastext"));
    });

    s.add_test("currentcolor css keyword", [](etest::IActions &a) {
        dom::Node dom_node = dom::Element{"dummy"s};
        style::StyledNode root{
                .node = dom_node,
                .properties = {{css::PropertyId::Color, "blue"s}},
                .children{
                        style::StyledNode{
                                .node{dom_node},
                                .properties{
                                        {css::PropertyId::BackgroundColor, "currentcolor"s},
                                },
                        },
                },
        };

        auto &child = root.children[0];
        child.parent = &root;

        a.expect_eq(child.get_property<css::PropertyId::BackgroundColor>(), gfx::Color::from_css_name("blue"));

        // "color: currentcolor" should be treated as inherit.
        child.properties.emplace_back(css::PropertyId::Color, "currentcolor"s);
        a.expect_eq(child.get_property<css::PropertyId::Color>(), gfx::Color::from_css_name("blue"));
    });

    s.add_test("get_font_style_property", [](etest::IActions &a) {
        expect_property_eq<css::PropertyId::FontStyle>(a, "oblique", style::FontStyle::Oblique);

        // Unhandled properties don't break things.
        expect_property_eq<css::PropertyId::FontStyle>(a, "???", style::FontStyle::Normal);
    });

    s.add_test("get_font_family_property", [](etest::IActions &a) {
        using FontFamilies = std::vector<std::string_view>;
        expect_property_eq<css::PropertyId::FontFamily>(a, "abc, def", FontFamilies{"abc", "def"});
        expect_property_eq<css::PropertyId::FontFamily>(a, R"('abc', "def")", FontFamilies{"abc", "def"});
        expect_property_eq<css::PropertyId::FontFamily>(a, "arial", FontFamilies{"arial"});
        expect_property_eq<css::PropertyId::FontFamily>(a, "'arial'", FontFamilies{"arial"});
        expect_property_eq<css::PropertyId::FontFamily>(a, R"("arial")", FontFamilies{"arial"});
    });

    s.add_test("get_font_size_property", [](etest::IActions &a) {
        dom::Node dom_node = dom::Element{"dummy"s};
        style::StyledNode root{
                .node = dom_node,
                .properties = {{css::PropertyId::FontSize, "50px"s}},
                .children{
                        style::StyledNode{
                                .node{dom_node},
                                .properties{{css::PropertyId::FontSize, "10px"s}},
                        },
                },
        };
        int default_font_size = style::StyledNode{dom_node, {}, {}}.get_property<css::PropertyId::FontSize>();

        auto &child = root.children[0];
        child.parent = &root;

        // px
        a.expect_eq(child.get_property<css::PropertyId::FontSize>(), 10);
        a.expect_eq(root.get_property<css::PropertyId::FontSize>(), 50);

        // inherit, unset
        child.properties[0] = {css::PropertyId::FontSize, "unset"};
        a.expect_eq(child.get_property<css::PropertyId::FontSize>(), 50);
        child.properties[0] = {css::PropertyId::FontSize, "inherit"};
        a.expect_eq(child.get_property<css::PropertyId::FontSize>(), 50);

        // %
        child.properties[0] = {css::PropertyId::FontSize, "100%"};
        a.expect_eq(child.get_property<css::PropertyId::FontSize>(), 50);
        child.properties[0] = {css::PropertyId::FontSize, "50%"};
        a.expect_eq(child.get_property<css::PropertyId::FontSize>(), 25);

        child.properties[0] = {css::PropertyId::FontSize, "larger"};
        a.expect(child.get_property<css::PropertyId::FontSize>() > 50);
        child.properties[0] = {css::PropertyId::FontSize, "smaller"};
        a.expect(child.get_property<css::PropertyId::FontSize>() < 50);

        // ex
        child.properties[0] = {css::PropertyId::FontSize, "1ex"};
        a.expect_eq(child.get_property<css::PropertyId::FontSize>(), 25);

        // ch
        child.properties[0] = {css::PropertyId::FontSize, "1ch"};
        a.expect_eq(child.get_property<css::PropertyId::FontSize>(), 25);

        // rem
        auto &child2 = child.children.emplace_back(
                style::StyledNode{.node{dom_node}, .properties{{css::PropertyId::FontSize, "2rem"}}, .parent = &child});
        a.expect_eq(child2.get_property<css::PropertyId::FontSize>(), 50 * 2);
        child2.properties[0] = {css::PropertyId::FontSize, "0.5rem"};
        a.expect_eq(child2.get_property<css::PropertyId::FontSize>(), 25);

        // em
        child.properties[0] = {css::PropertyId::FontSize, "2em"};
        a.expect_eq(child.get_property<css::PropertyId::FontSize>(), 50 * 2);
        root.properties[0] = {css::PropertyId::FontSize, "25px"};
        a.expect_eq(child.get_property<css::PropertyId::FontSize>(), 25 * 2);
        root.properties[0] = {css::PropertyId::FontSize, "2em"};
        a.expect_eq(child.get_property<css::PropertyId::FontSize>(), default_font_size * 2 * 2);
        root.properties.clear();
        a.expect_eq(child.get_property<css::PropertyId::FontSize>(), default_font_size * 2);

        // unhandled units
        child.properties[0] = {css::PropertyId::FontSize, "1asdf"};
        a.expect_eq(child.get_property<css::PropertyId::FontSize>(), 0);

        // 0
        child.properties[0] = {css::PropertyId::FontSize, "0"};
        a.expect_eq(child.get_property<css::PropertyId::FontSize>(), 0);

        // pt
        expect_property_eq<css::PropertyId::FontSize>(a, "12pt", 16);
        expect_property_eq<css::PropertyId::FontSize>(a, "24pt", 32);

        // Invalid, shouldn't crash.
        // TODO(robinlinden): Make this do whatever other browsers do.
        child.properties[0] = {css::PropertyId::FontSize, "abcd"};
        std::ignore = child.get_property<css::PropertyId::FontSize>();
    });

    s.add_test("xpath", [](etest::IActions &a) {
        dom::Node html_node = dom::Element{"html"s};
        dom::Node div_node = dom::Element{"div"s};
        dom::Node text_node = dom::Text{"hello!"s};
        style::StyledNode styled_node{
                .node = html_node,
                .children{
                        style::StyledNode{.node = text_node},
                        style::StyledNode{.node = div_node},
                },
        };

        using NodeVec = std::vector<style::StyledNode const *>;
        a.expect_eq(dom::nodes_by_xpath(styled_node, "/html"), NodeVec{&styled_node});
        a.expect_eq(dom::nodes_by_xpath(styled_node, "/html/div"), NodeVec{&styled_node.children[1]});
        a.expect_eq(dom::nodes_by_xpath(styled_node, "/html/div/"), NodeVec{});
        a.expect_eq(dom::nodes_by_xpath(styled_node, "/html/div/p"), NodeVec{});
        a.expect_eq(dom::nodes_by_xpath(styled_node, "/htm/div"), NodeVec{});
        a.expect_eq(dom::nodes_by_xpath(styled_node, "//div"), NodeVec{&styled_node.children[1]});
    });

    s.add_test("get_property, last property gets priority", [](etest::IActions &a) {
        dom::Node dom_node = dom::Element{"dummy"s};
        style::StyledNode styled_node{
                .node = dom_node,
                .properties = {{css::PropertyId::Display, "block"s}, {css::PropertyId::Display, "none"}},
                .children = {},
        };

        a.expect_eq(styled_node.get_property<css::PropertyId::Display>(), std::nullopt);
    });

    s.add_test("get_property, display", [](etest::IActions &a) {
        using style::Display;
        expect_property_eq<css::PropertyId::Display>(a, "inline", Display::inline_flow());
        expect_property_eq<css::PropertyId::Display>(a, "i cant believe this", Display::block_flow());

        // Weird float interactions.
        dom::Node dom_node = dom::Element{"dummy"s};
        style::StyledNode styled{
                .node = dom_node,
                .properties{
                        {css::PropertyId::Display, "???"s},
                        {css::PropertyId::Float, "left"s},
                },
        };

        styled.properties[0] = {css::PropertyId::Display, "inline"s};
        a.expect_eq(styled.get_property<css::PropertyId::Display>(), Display::block_flow());
    });

    s.add_test("get_property, border-style", [](etest::IActions &a) {
        expect_property_eq<css::PropertyId::BorderBottomStyle>(a, "none", style::BorderStyle::None);
        expect_property_eq<css::PropertyId::BorderBottomStyle>(a, "hidden", style::BorderStyle::Hidden);
        expect_property_eq<css::PropertyId::BorderBottomStyle>(a, "dotted", style::BorderStyle::Dotted);
        expect_property_eq<css::PropertyId::BorderBottomStyle>(a, "dashed", style::BorderStyle::Dashed);
        expect_property_eq<css::PropertyId::BorderBottomStyle>(a, "solid", style::BorderStyle::Solid);
        expect_property_eq<css::PropertyId::BorderBottomStyle>(a, "double", style::BorderStyle::Double);
        expect_property_eq<css::PropertyId::BorderBottomStyle>(a, "groove", style::BorderStyle::Groove);
        expect_property_eq<css::PropertyId::BorderBottomStyle>(a, "ridge", style::BorderStyle::Ridge);
        expect_property_eq<css::PropertyId::BorderBottomStyle>(a, "inset", style::BorderStyle::Inset);
        expect_property_eq<css::PropertyId::BorderBottomStyle>(a, "outset", style::BorderStyle::Outset);
        expect_property_eq<css::PropertyId::BorderBottomStyle>(a, "???", style::BorderStyle::None);

        expect_property_eq<css::PropertyId::BorderLeftStyle>(a, "ridge", style::BorderStyle::Ridge);
        expect_property_eq<css::PropertyId::BorderRightStyle>(a, "ridge", style::BorderStyle::Ridge);
        expect_property_eq<css::PropertyId::BorderTopStyle>(a, "ridge", style::BorderStyle::Ridge);
    });

    s.add_test("get_property, outline-style", [](etest::IActions &a) {
        expect_property_eq<css::PropertyId::OutlineStyle>(a, "none", style::BorderStyle::None);
        expect_property_eq<css::PropertyId::OutlineStyle>(a, "hidden", style::BorderStyle::Hidden);
        expect_property_eq<css::PropertyId::OutlineStyle>(a, "dotted", style::BorderStyle::Dotted);
        expect_property_eq<css::PropertyId::OutlineStyle>(a, "dashed", style::BorderStyle::Dashed);
        expect_property_eq<css::PropertyId::OutlineStyle>(a, "solid", style::BorderStyle::Solid);
        expect_property_eq<css::PropertyId::OutlineStyle>(a, "double", style::BorderStyle::Double);
        expect_property_eq<css::PropertyId::OutlineStyle>(a, "groove", style::BorderStyle::Groove);
        expect_property_eq<css::PropertyId::OutlineStyle>(a, "ridge", style::BorderStyle::Ridge);
        expect_property_eq<css::PropertyId::OutlineStyle>(a, "inset", style::BorderStyle::Inset);
        expect_property_eq<css::PropertyId::OutlineStyle>(a, "outset", style::BorderStyle::Outset);
        expect_property_eq<css::PropertyId::OutlineStyle>(a, "???", style::BorderStyle::None);
    });

    s.add_test("get_property, outline-color", [](etest::IActions &a) {
        expect_property_eq<css::PropertyId::Color>(a, "rgba(1 2 3)", gfx::Color{1, 2, 3});
        expect_property_eq<css::PropertyId::Color>(a, "rgba(1 2 3 / .5)", gfx::Color{1, 2, 3, 127});
        expect_property_eq<css::PropertyId::Color>(a, "rgba(1 2 3 / -0.5)", gfx::Color{1, 2, 3, 0});
        expect_property_eq<css::PropertyId::Color>(a, "rgba(1 2 3 / 1.5)", gfx::Color{1, 2, 3, 0xFF});
    });

    s.add_test("get_property, color", [](etest::IActions &a) {
        expect_property_eq<css::PropertyId::Color>(a, "rgba(1 2 3)", gfx::Color{1, 2, 3});
        expect_property_eq<css::PropertyId::Color>(a, "rgba(1 2 3 / .5)", gfx::Color{1, 2, 3, 127});
        expect_property_eq<css::PropertyId::Color>(a, "rgba(1 2 3 / -0.5)", gfx::Color{1, 2, 3, 0});
        expect_property_eq<css::PropertyId::Color>(a, "rgba(1 2 3 / 1.5)", gfx::Color{1, 2, 3, 0xFF});

        // Invalid syntax.
        constexpr gfx::Color kErrorColor{0xFF, 0, 0};
        expect_property_eq<css::PropertyId::Color>(a, "rgba(1 2)", kErrorColor);
    });

    s.add_test("get_property, float", [](etest::IActions &a) {
        expect_property_eq<css::PropertyId::Float>(a, "none", style::Float::None);
        expect_property_eq<css::PropertyId::Float>(a, "left", style::Float::Left);
        expect_property_eq<css::PropertyId::Float>(a, "right", style::Float::Right);
        expect_property_eq<css::PropertyId::Float>(a, "inline-start", style::Float::InlineStart);
        expect_property_eq<css::PropertyId::Float>(a, "inline-end", style::Float::InlineEnd);
        expect_property_eq<css::PropertyId::Float>(a, "???", std::nullopt);
    });

    s.add_test("get_property, text-align", [](etest::IActions &a) {
        using enum style::TextAlign;
        expect_property_eq<css::PropertyId::TextAlign>(a, "left", Left);
        expect_property_eq<css::PropertyId::TextAlign>(a, "right", Right);
        expect_property_eq<css::PropertyId::TextAlign>(a, "center", Center);
        expect_property_eq<css::PropertyId::TextAlign>(a, "justify", Justify);

        expect_property_eq<css::PropertyId::TextAlign>(a, "unhandled!", Left);
    });

    s.add_test("get_property, text-decoration-line", [](etest::IActions &a) {
        using enum style::TextDecorationLine;
        expect_property_eq<css::PropertyId::TextDecorationLine>(a, "none", std::vector{None});
        expect_property_eq<css::PropertyId::TextDecorationLine>(a, "underline", std::vector{Underline});
        expect_property_eq<css::PropertyId::TextDecorationLine>(a, "overline", std::vector{Overline});
        expect_property_eq<css::PropertyId::TextDecorationLine>(a, "line-through", std::vector{LineThrough});
        expect_property_eq<css::PropertyId::TextDecorationLine>(
                a, "underline overline", std::vector{Underline, Overline});

        expect_property_eq<css::PropertyId::TextDecorationLine>(a, "blink", std::vector<style::TextDecorationLine>{});
        expect_property_eq<css::PropertyId::TextDecorationLine>(
                a, "unhandled!", std::vector<style::TextDecorationLine>{});
    });

    s.add_test("get_property, text-transform", [](etest::IActions &a) {
        using enum style::TextTransform;
        expect_property_eq<css::PropertyId::TextTransform>(a, "none", None);
        expect_property_eq<css::PropertyId::TextTransform>(a, "capitalize", Capitalize);
        expect_property_eq<css::PropertyId::TextTransform>(a, "uppercase", Uppercase);
        expect_property_eq<css::PropertyId::TextTransform>(a, "lowercase", Lowercase);
        expect_property_eq<css::PropertyId::TextTransform>(a, "full-width", FullWidth);
        expect_property_eq<css::PropertyId::TextTransform>(a, "full-size-kana", FullSizeKana);

        expect_property_eq<css::PropertyId::TextTransform>(a, "unhandled!", std::nullopt);
    });

    s.add_test("get_property, white-space", [](etest::IActions &a) {
        expect_property_eq<css::PropertyId::WhiteSpace>(a, "normal", style::WhiteSpace::Normal);
        expect_property_eq<css::PropertyId::WhiteSpace>(a, "pre", style::WhiteSpace::Pre);
        expect_property_eq<css::PropertyId::WhiteSpace>(a, "nowrap", style::WhiteSpace::Nowrap);
        expect_property_eq<css::PropertyId::WhiteSpace>(a, "pre-wrap", style::WhiteSpace::PreWrap);
        expect_property_eq<css::PropertyId::WhiteSpace>(a, "break-spaces", style::WhiteSpace::BreakSpaces);
        expect_property_eq<css::PropertyId::WhiteSpace>(a, "pre-line", style::WhiteSpace::PreLine);

        expect_property_eq<css::PropertyId::WhiteSpace>(a, "unhandled!", std::nullopt);
    });

    s.add_test("get_property, non-inherited property for a text node", [](etest::IActions &a) {
        dom::Node dom = dom::Element{"hello"};
        dom::Node text = dom::Text{"world"};
        style::StyledNode styled_node{
                .node = dom,
                .properties{
                        {css::PropertyId::TextDecorationLine, "overline"s},
                        {css::PropertyId::Display, "block"s},
                },
        };
        auto const &child = styled_node.children.emplace_back(style::StyledNode{.node = text, .parent = &styled_node});

        a.expect(!css::is_inherited(css::PropertyId::TextDecorationLine));
        a.expect_eq(child.get_property<css::PropertyId::TextDecorationLine>(),
                std::vector{style::TextDecorationLine::Overline});

        // Text is always "display: inline".
        a.expect_eq(child.get_property<css::PropertyId::Display>(), style::Display::inline_flow());
    });

    s.add_test("get_property, font-weight", [](etest::IActions &a) {
        expect_property_eq<css::PropertyId::FontWeight>(a, "normal", style::FontWeight::normal());
        expect_property_eq<css::PropertyId::FontWeight>(a, "bold", style::FontWeight::bold());

        expect_property_eq<css::PropertyId::FontWeight>(a, "123", style::FontWeight{123});

        expect_relative_property_eq<css::PropertyId::FontWeight>(a, "bolder", "50", style::FontWeight::normal());
        expect_relative_property_eq<css::PropertyId::FontWeight>(a, "bolder", "normal", style::FontWeight::bold());
        expect_relative_property_eq<css::PropertyId::FontWeight>(a, "bolder", "bold", style::FontWeight{900});
        expect_relative_property_eq<css::PropertyId::FontWeight>(a, "bolder", "999", style::FontWeight{999});

        expect_relative_property_eq<css::PropertyId::FontWeight>(a, "lighter", "50", style::FontWeight{50});
        expect_relative_property_eq<css::PropertyId::FontWeight>(a, "lighter", "normal", style::FontWeight{100});
        expect_relative_property_eq<css::PropertyId::FontWeight>(a, "lighter", "bold", style::FontWeight::normal());
        expect_relative_property_eq<css::PropertyId::FontWeight>(a, "lighter", "999", style::FontWeight::bold());

        // Invalid values.
        expect_property_eq<css::PropertyId::FontWeight>(a, "???", std::nullopt);
        expect_property_eq<css::PropertyId::FontWeight>(a, "0", std::nullopt);
        expect_property_eq<css::PropertyId::FontWeight>(a, "1001", std::nullopt);
        expect_property_eq<css::PropertyId::FontWeight>(a, "500px", std::nullopt);

        // Relative, no parent
        dom::Node dom = dom::Element{"baka"};
        style::StyledNode styled_node{.node = dom, .properties = {{css::PropertyId::FontWeight, "bolder"s}}};
        a.expect_eq(styled_node.get_property<css::PropertyId::FontWeight>(), style::FontWeight::bold());
        styled_node.properties[0] = {css::PropertyId::FontWeight, "lighter"s};
        a.expect_eq(styled_node.get_property<css::PropertyId::FontWeight>(), style::FontWeight{100});
    });

    s.add_test("var", [](etest::IActions &a) {
        dom::Node dom = dom::Element{"baka"};
        style::StyledNode styled_node{
                .node = dom,
                .properties{
                        {css::PropertyId::Color, "var(--color)"s},
                        {css::PropertyId::FontWeight, "var(--weight)"s},
                },
                .custom_properties = {{"--color", "#abc"s}},
        };

        a.expect_eq(styled_node.get_property<css::PropertyId::Color>(), //
                gfx::Color{0xaa, 0xbb, 0xcc});

        // Unresolved variables return the initial value.
        a.expect_eq(css::initial_value(css::PropertyId::FontWeight), "normal");
        a.expect_eq(styled_node.get_property<css::PropertyId::FontWeight>(), //
                style::FontWeight::normal());

        styled_node.custom_properties = {{"--weight", "bold"}};
        a.expect_eq(styled_node.get_property<css::PropertyId::FontWeight>(), //
                style::FontWeight::bold());
    });

    s.add_test("font-size: var", [](etest::IActions &a) {
        dom::Node dom = dom::Element{"baka"};
        style::StyledNode styled_node{
                .node = dom,
                .properties{{css::PropertyId::FontSize, "var(--size)"}},
                .custom_properties = {{"--size", "37px"}},
        };

        a.expect_eq(styled_node.get_property<css::PropertyId::FontSize>(), 37);

        styled_node.custom_properties = {};
        auto const font_size_on_resolution_failure = styled_node.get_property<css::PropertyId::FontSize>();
        a.expect(font_size_on_resolution_failure != 37);

        // Unresolved variables return the initial value.
        style::StyledNode b{.node = dom};
        a.expect_eq(b.get_property<css::PropertyId::FontSize>(), font_size_on_resolution_failure);
    });

    s.add_test("var(var)", [](etest::IActions &a) {
        dom::Node dom = dom::Element{"baka"};
        style::StyledNode styled_node{
                .node = dom,
                .properties{{css::PropertyId::FontWeight, "var(--a)"}},
                .custom_properties{
                        {"--a", "var(--b)"},
                        {"--b", "bold"},
                },
        };

        a.expect_eq(styled_node.get_property<css::PropertyId::FontWeight>(), //
                style::FontWeight::bold());

        // Circular references are bad.
        styled_node.custom_properties = {
                {"--a", "var(--b)"},
                {"--b", "var(--a)"},
        };
        a.expect_eq(styled_node.get_property<css::PropertyId::FontWeight>(), //
                style::FontWeight::normal());
    });

    s.add_test("var() with fallback, var exists", [](etest::IActions &a) {
        dom::Node dom = dom::Element{"baka"};
        style::StyledNode styled_node{
                .node = dom,
                .properties{{css::PropertyId::FontWeight, "var(--a, 789)"}},
                .custom_properties{{"--a", "123"}},
        };

        a.expect_eq(styled_node.get_property<css::PropertyId::FontWeight>()->value, 123);
    });

    s.add_test("var() with var() fallback", [](etest::IActions &a) {
        dom::Node dom = dom::Element{"baka"};
        style::StyledNode styled_node{
                .node = dom,
                .properties{{css::PropertyId::FontWeight, "var(--a, var(--b, 789))"}},
                .custom_properties{{"--a", "123"}},
        };

        a.expect_eq(styled_node.get_property<css::PropertyId::FontWeight>()->value, 123);

        styled_node.custom_properties = {{"--b", "456"}};
        a.expect_eq(styled_node.get_property<css::PropertyId::FontWeight>()->value, 456);

        styled_node.custom_properties = {};
        a.expect_eq(styled_node.get_property<css::PropertyId::FontWeight>()->value, 789);

        styled_node.custom_properties = {{"--a", "var(--c, var(--b))"}, {"--b", "888"}};
        a.expect_eq(styled_node.get_property<css::PropertyId::FontWeight>()->value, 888);

        // Silly circular reference, should return the initial value.
        styled_node.custom_properties = {{"--a", "var(--c, var(--a))"}};
        a.expect_eq(styled_node.get_property<css::PropertyId::FontWeight>()->value, style::FontWeight::normal().value);
    });

    s.add_test("var() with fallback, no var exists", [](etest::IActions &a) {
        dom::Node dom = dom::Element{"baka"};
        style::StyledNode styled_node{
                .node = dom,
                .properties{{css::PropertyId::FontWeight, "var(--a, 789)"}},
        };

        a.expect_eq(styled_node.get_property<css::PropertyId::FontWeight>()->value, 789);
    });

    s.add_test("var, inherited custom property", [](etest::IActions &a) {
        dom::Node dom = dom::Element{"baka"};
        style::StyledNode styled_node{
                .node = dom,
                .children{
                        style::StyledNode{
                                .node{dom},
                                .properties{{css::PropertyId::FontWeight, "var(--a)"}},
                        },
                },
                .custom_properties{{"--a", "bold"}},
        };

        auto &child = styled_node.children[0];
        child.parent = &styled_node;

        a.expect_eq(child.get_property<css::PropertyId::FontWeight>(), //
                style::FontWeight::bold());
    });

    s.add_test("==, custom properties", [](etest::IActions &a) {
        dom::Node dom = dom::Element{"baka"};

        style::StyledNode with{
                .node = dom,
                .custom_properties{{"--a", "bold"}},
        };

        style::StyledNode without{.node = dom};

        a.expect(with != without);

        without.custom_properties = {{"--a", "bold"}};
        a.expect_eq(with, without);
    });

    s.add_test("border radius", [](etest::IActions &a) {
        expect_property_eq<BorderTopLeftRadius>(a, "2em", std::pair{60, 60}, {{FontSize, "30px"}});
        expect_property_eq<BorderTopRightRadius>(a, std::nullopt, std::pair{0, 0});
        expect_property_eq<BorderBottomLeftRadius>(a, std::nullopt, std::pair{0, 0});
        expect_property_eq<BorderBottomRightRadius>(a, "10px/3em", std::pair{10, 90}, {{FontSize, "30px"}});
    });

    s.add_test("width", [](etest::IActions &a) {
        expect_property_eq<MinWidth>(a, "13px", style::UnresolvedValue{"13px"});
        expect_property_eq<MinWidth>(a, "auto", style::UnresolvedValue{"auto"});

        expect_property_eq<Width>(a, "42px", style::UnresolvedValue{"42px"});
        expect_property_eq<Width>(a, "auto", style::UnresolvedValue{"auto"});

        expect_property_eq<MaxWidth>(a, "420px", style::UnresolvedValue{"420px"});
        expect_property_eq<MaxWidth>(a, "none", style::UnresolvedValue{"none"});
    });

    return s.run();
}
