// SPDX-FileCopyrightText: 2021-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "style/style.h"
#include "style/styled_node.h"

#include "css/media_query.h"
#include "css/property_id.h"
#include "css/rule.h"
#include "css/style_sheet.h"
#include "dom/dom.h"
#include "etest/etest.h"

#include <fmt/core.h>

#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace std::literals;
using etest::expect;
using etest::expect_eq;
using etest::require;

namespace {
bool is_match(dom::Element const &e, std::string_view selector) {
    return is_match(style::StyledNode{e}, selector);
}

std::vector<std::pair<css::PropertyId, std::string>> matching_rules(
        dom::Element const &element, css::StyleSheet const &stylesheet, css::MediaQuery::Context const &context = {}) {
    return matching_rules(style::StyledNode{element}, stylesheet, context);
}

bool check_parents(style::StyledNode const &a, style::StyledNode const &b) {
    if (!std::ranges::equal(a.children, b.children, &check_parents)) {
        return false;
    }

    if (a.parent == nullptr || b.parent == nullptr) {
        return a.parent == b.parent;
    }

    return *a.parent == *b.parent;
}

void inline_css_tests() {
    etest::test("inline css: is applied", [] {
        dom::Node dom = dom::Element{"div", {{"style", {"font-size:2px"}}}};
        auto styled = style::style_tree(dom, {}, {});
        expect_eq(styled->properties, std::vector{std::pair{css::PropertyId::FontSize, "2px"s}});
    });

    etest::test("inline css: overrides the stylesheet", [] {
        dom::Node dom = dom::Element{"div", {{"style", {"font-size:2px"}}}};
        auto styled = style::style_tree(dom, {{css::Rule{{"div"}, {{css::PropertyId::FontSize, "2000px"}}}}}, {});

        // The last property is the one that's applied.
        expect_eq(styled->properties,
                std::vector{
                        std::pair{css::PropertyId::FontSize, "2000px"s}, std::pair{css::PropertyId::FontSize, "2px"s}});
    });
}

void important_declarations_tests() {
    etest::test("!important: has higher priority", [] {
        dom::Node dom = dom::Element{"div"};
        css::StyleSheet css{.rules{{
                .selectors = {"div"},
                .declarations = {{css::PropertyId::FontSize, "2px"}},
                .important_declarations = {{css::PropertyId::FontSize, "20px"}},
        }}};
        auto styled = style::style_tree(dom, css);

        // The last property is the one that's applied.
        expect_eq(styled->properties,
                std::vector{
                        std::pair{css::PropertyId::FontSize, "2px"s},
                        std::pair{css::PropertyId::FontSize, "20px"s},
                });
    });
}
} // namespace

int main() {
    etest::test("is_match: universal selector", [] {
        expect(is_match(dom::Element{"div"}, "*"sv));
        expect(is_match(dom::Element{"span"}, "*"sv));
    });

    etest::test("is_match: simple names", [] {
        expect(is_match(dom::Element{"div"}, "div"sv));
        expect(!is_match(dom::Element{"div"}, "span"sv));
    });

    etest::test("is_match: class", [] {
        expect(!is_match(dom::Element{"div"}, ".myclass"sv));
        expect(!is_match(dom::Element{"div", {{"id", "myclass"}}}, ".myclass"sv));
        expect(is_match(dom::Element{"div", {{"class", "myclass"}}}, ".myclass"sv));
        expect(is_match(dom::Element{"div", {{"class", "first second"}}}, ".first"sv));
        expect(is_match(dom::Element{"div", {{"class", "first second"}}}, ".second"sv));
    });

    etest::test("is_match: id", [] {
        expect(!is_match(dom::Element{"div"}, "#myid"sv));
        expect(is_match(dom::Element{"div", {{"class", "myid"}}}, ".myid"sv));
        expect(!is_match(dom::Element{"div", {{"id", "myid"}}}, ".myid"sv));
    });

    etest::test("is_match: psuedo-class, unhandled", [] {
        expect(!is_match(dom::Element{"div"}, ":hi"sv));
        expect(!is_match(dom::Element{"div"}, "div:hi"sv));
    });

    // These are 100% identical right now as we treat all links as unvisited links.
    for (auto const *pc : std::array{"link", "any-link"}) {
        etest::test(fmt::format("is_match: psuedo-class, {}", pc), [pc] {
            expect(is_match(dom::Element{"a", {{"href", ""}}}, fmt::format(":{}", pc)));

            expect(is_match(dom::Element{"a", {{"href", ""}}}, fmt::format("a:{}", pc)));
            expect(is_match(dom::Element{"area", {{"href", ""}}}, fmt::format("area:{}", pc)));

            expect(is_match(dom::Element{"a", {{"href", ""}, {"class", "hi"}}}, fmt::format(".hi:{}", pc)));
            expect(is_match(dom::Element{"a", {{"href", ""}, {"id", "hi"}}}, fmt::format("#hi:{}", pc)));

            expect(!is_match(dom::Element{"b"}, fmt::format(":{}", pc)));
            expect(!is_match(dom::Element{"a"}, fmt::format("a:{}", pc)));
            expect(!is_match(dom::Element{"a", {{"href", ""}}}, fmt::format("b:{}", pc)));
            expect(!is_match(dom::Element{"b", {{"href", ""}}}, fmt::format("b:{}", pc)));
            expect(!is_match(dom::Element{"a", {{"href", ""}, {"class", "hi2"}}}, fmt::format(".hi:{}", pc)));
            expect(!is_match(dom::Element{"a", {{"href", ""}, {"id", "hi2"}}}, fmt::format("#hi:{}", pc)));
        });
    }

    etest::test("is_match: :root", [] {
        dom::Element dom = dom::Element{"html", {}, {dom::Element{"body"}}};
        style::StyledNode node{dom, {}, {style::StyledNode{dom.children[0]}}};
        node.children[0].parent = &node;

        expect(style::is_match(node, ":root"));
        expect(!style::is_match(node.children[0], ":root"));
    });

    etest::test("is_match: child", [] {
        dom::Element dom = dom::Element{"div", {{"class", "logo"}}, {dom::Element{"span"}}};
        style::StyledNode node{dom, {}, {style::StyledNode{dom.children[0]}}};
        node.children[0].parent = &node;
        expect(style::is_match(node.children[0], ".logo > span"sv));
        expect(!style::is_match(node, ".logo > span"sv));

        std::get<dom::Element>(dom.children[0]).attributes["class"] = "ohno";
        expect(style::is_match(node.children[0], ".logo > .ohno"sv));
        expect(style::is_match(node.children[0], ".logo > span"sv));
    });

    etest::test("is_match: descendant", [] {
        using style::StyledNode;
        // DOM for div[.logo] { span { a } }
        dom::Element dom = dom::Element{"div", {{"class", "logo"}}, {dom::Element{"span", {}, {dom::Element{"a"}}}}};
        StyledNode node{dom, {}, {{dom.children[0], {}, {{std::get<dom::Element>(dom.children[0]).children[0]}}}}};
        node.children[0].parent = &node;
        node.children[0].children[0].parent = &node.children[0];

        expect(style::is_match(node.children[0], ".logo span"sv));
        expect(style::is_match(node.children[0], "div span"sv));
        expect(!style::is_match(node, ".logo span"sv));

        std::get<dom::Element>(dom.children[0]).attributes["class"] = "ohno";
        expect(style::is_match(node.children[0], ".logo .ohno"sv));
        expect(style::is_match(node.children[0], ".logo span"sv));

        expect(style::is_match(node.children[0].children[0], "div a"sv));
        expect(style::is_match(node.children[0].children[0], ".logo a"sv));
        expect(style::is_match(node.children[0].children[0], "span a"sv));
        expect(style::is_match(node.children[0].children[0], ".ohno a"sv));
        expect(style::is_match(node.children[0].children[0], "div span a"sv));
        expect(style::is_match(node.children[0].children[0], ".logo span a"sv));
        expect(style::is_match(node.children[0].children[0], "div .ohno a"sv));
        expect(style::is_match(node.children[0].children[0], ".logo .ohno a"sv));
    });

    etest::test("matching_rules: simple names", [] {
        css::StyleSheet stylesheet;
        expect(matching_rules(dom::Element{"div"}, stylesheet).empty());

        stylesheet.rules.push_back(
                css::Rule{.selectors = {"span", "p"}, .declarations = {{css::PropertyId::Width, "80px"}}});

        expect(matching_rules(dom::Element{"div"}, stylesheet).empty());

        {
            auto span_rules = matching_rules(dom::Element{"span"}, stylesheet);
            require(span_rules.size() == 1);
            expect(span_rules[0] == std::pair{css::PropertyId::Width, "80px"s});
        }

        {
            auto p_rules = matching_rules(dom::Element{"p"}, stylesheet);
            require(p_rules.size() == 1);
            expect(p_rules[0] == std::pair{css::PropertyId::Width, "80px"s});
        }

        stylesheet.rules.push_back(
                css::Rule{.selectors = {"span", "hr"}, .declarations = {{css::PropertyId::Height, "auto"}}});

        expect(matching_rules(dom::Element{"div"}, stylesheet).empty());

        {
            auto span_rules = matching_rules(dom::Element{"span"}, stylesheet);
            require(span_rules.size() == 2);
            expect(span_rules[0] == std::pair{css::PropertyId::Width, "80px"s});
            expect(span_rules[1] == std::pair{css::PropertyId::Height, "auto"s});
        }

        {
            auto p_rules = matching_rules(dom::Element{"p"}, stylesheet);
            require(p_rules.size() == 1);
            expect(p_rules[0] == std::pair{css::PropertyId::Width, "80px"s});
        }

        {
            auto hr_rules = matching_rules(dom::Element{"hr"}, stylesheet);
            require(hr_rules.size() == 1);
            expect(hr_rules[0] == std::pair{css::PropertyId::Height, "auto"s});
        }
    });

    etest::test("matching_rules: media query", [] {
        css::StyleSheet stylesheet{{
                css::Rule{.selectors{"p"}, .declarations{{css::PropertyId::Color, "red"}}},
        }};

        expect_eq(
                matching_rules(dom::Element{"p"}, stylesheet), std::vector{std::pair{css::PropertyId::Color, "red"s}});

        stylesheet.rules[0].media_query = css::MediaQuery::parse("(min-width: 700px)");
        expect(matching_rules(dom::Element{"p"}, stylesheet).empty());

        expect_eq(matching_rules(dom::Element{"p"}, stylesheet, {.window_width = 700}),
                std::vector{std::pair{css::PropertyId::Color, "red"s}});
    });

    etest::test("style_tree: structure", [] {
        auto root = dom::Element{"html", {}, {}};
        root.children.emplace_back(dom::Element{"head"});
        root.children.emplace_back(dom::Element{"body", {}, {dom::Element{"p"}}});

        style::StyledNode expected{root};
        expected.children.push_back({root.children[0], {}, {}, &expected});
        expected.children.push_back({root.children[1], {}, {}, &expected});

        auto &body = expected.children.back();
        body.children.push_back({std::get<dom::Element>(root.children[1]).children[0], {}, {}, &body});

        expect(*style::style_tree(root, {}) == expected);
        expect(check_parents(*style::style_tree(root, {}), expected));
    });

    etest::test("style_tree: style is applied", [] {
        auto root = dom::Element{"html", {}, {}};
        root.children.emplace_back(dom::Element{"head"});
        root.children.emplace_back(dom::Element{"body", {}, {dom::Element{"p"}}});

        css::StyleSheet stylesheet{{
                {.selectors = {"p"}, .declarations = {{css::PropertyId::Height, "100px"}}},
                {.selectors = {"body"}, .declarations = {{css::PropertyId::FontSize, "500em"}}},
        }};

        style::StyledNode expected{root};
        expected.children.push_back({root.children[0], {}, {}, &expected});
        expected.children.push_back({root.children[1], {{css::PropertyId::FontSize, "500em"}}, {}, &expected});
        auto &body = expected.children.back();

        body.children.push_back({std::get<dom::Element>(root.children[1]).children[0],
                {{css::PropertyId::Height, "100px"}},
                {},
                &body});

        expect(*style::style_tree(root, stylesheet) == expected);
        expect(check_parents(*style::style_tree(root, stylesheet), expected));
    });

    inline_css_tests();
    important_declarations_tests();
    return etest::run_all_tests();
}
