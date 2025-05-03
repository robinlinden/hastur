// SPDX-FileCopyrightText: 2021-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "style/style.h"
#include "style/styled_node.h"

#include "css/media_query.h"
#include "css/property_id.h"
#include "css/rule.h"
#include "css/style_sheet.h"
#include "dom/dom.h"
#include "etest/etest2.h"

#include <algorithm>
#include <array>
#include <format>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

using namespace std::literals;

namespace {
bool is_match(dom::Element const &e, std::string_view selector) {
    return is_match(style::StyledNode{e}, selector);
}

std::vector<std::pair<css::PropertyId, std::string>> matching_rules(
        dom::Element const &element, css::StyleSheet const &stylesheet, css::MediaQuery::Context const &context = {}) {
    return matching_properties(style::StyledNode{element}, stylesheet, context).normal;
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

void inline_css_tests(etest::Suite &s) {
    s.add_test("inline css: is applied", [](etest::IActions &a) {
        dom::Node dom = dom::Element{"div", {{"style", {"font-size:2px"}}}};
        auto styled = style::style_tree(dom, {}, {});
        a.expect_eq(styled->properties, std::vector{std::pair{css::PropertyId::FontSize, "2px"s}});
    });

    s.add_test("inline css: doesn't explode", [](etest::IActions &) {
        dom::Node dom = dom::Element{"div", {{"style", {"aaa"}}}};
        std::ignore = style::style_tree(dom, {}, {});
    });

    s.add_test("inline css: overrides the stylesheet", [](etest::IActions &a) {
        dom::Node dom = dom::Element{"div", {{"style", {"font-size:2px"}}}};
        auto styled = style::style_tree(dom, {{css::Rule{{"div"}, {{css::PropertyId::FontSize, "2000px"}}}}}, {});

        // The last property is the one that's applied.
        a.expect_eq(styled->properties,
                std::vector{
                        std::pair{css::PropertyId::FontSize, "2000px"s}, std::pair{css::PropertyId::FontSize, "2px"s}});
    });

    s.add_test("inline css: !important", [](etest::IActions &a) {
        dom::Node dom = dom::Element{"div", {{"style", {"font-size:2px !important"}}}};
        auto styled = style::style_tree(dom, {}, {});
        a.expect_eq(styled->properties, std::vector{std::pair{css::PropertyId::FontSize, "2px"s}});
    });
}

void important_declarations_tests(etest::Suite &s) {
    s.add_test("!important: has higher priority", [](etest::IActions &a) {
        dom::Node dom = dom::Element{"div"};
        css::StyleSheet css{.rules{{
                .selectors = {"div"},
                .declarations = {{css::PropertyId::FontSize, "2px"}},
                .important_declarations = {{css::PropertyId::FontSize, "20px"}},
        }}};
        auto styled = style::style_tree(dom, css);

        // The last property is the one that's applied.
        a.expect_eq(styled->properties,
                std::vector{
                        std::pair{css::PropertyId::FontSize, "2px"s},
                        std::pair{css::PropertyId::FontSize, "20px"s},
                });
    });
}

void attribute_selector_matching(etest::Suite &s) {
    s.add_test("is_match: attribute selector", [](etest::IActions &a) {
        a.expect(is_match(dom::Element{"p", {{"a", "b"}}}, "[a]"sv));
        a.expect(!is_match(dom::Element{"p", {{"a", "b"}}}, "[a"sv));
        a.expect(!is_match(dom::Element{"p"}, "[a]"sv));

        a.expect(is_match(dom::Element{"p", {{"a", "b"}}}, "[a=b]"sv));
        a.expect(!is_match(dom::Element{"p", {{"a", "b"}}}, "[a=c]"sv));

        a.expect(is_match(dom::Element{"p", {{"a", "b"}, {"c", "d"}}}, "[a=b][c=d]"sv));
        a.expect(is_match(dom::Element{"p", {{"a", "b"}, {"c", "d"}}}, "[a=b][c]"sv));
        a.expect(is_match(dom::Element{"p", {{"a", "b"}, {"c", "d"}}}, "[a][c]"sv));
        a.expect(!is_match(dom::Element{"p", {{"a", "b"}, {"c", "d"}}}, "[a=b][c=g]"sv));
        a.expect(!is_match(dom::Element{"p", {{"a", "b"}, {"c", "d"}}}, "[a=b][d]"sv));
        a.expect(!is_match(dom::Element{"p", {{"a", "b"}}}, "[a][c]"sv));
    });
}
} // namespace

int main() {
    etest::Suite s;

    s.add_test("is_match: universal selector", [](etest::IActions &a) {
        a.expect(is_match(dom::Element{"div"}, "*"sv));
        a.expect(is_match(dom::Element{"span"}, "*"sv));
    });

    s.add_test("is_match: simple names", [](etest::IActions &a) {
        a.expect(is_match(dom::Element{"div"}, "div"sv));
        a.expect(!is_match(dom::Element{"div"}, "span"sv));
    });

    s.add_test("is_match: class", [](etest::IActions &a) {
        a.expect(!is_match(dom::Element{"div"}, ".myclass"sv));
        a.expect(!is_match(dom::Element{"div", {{"id", "myclass"}}}, ".myclass"sv));
        a.expect(is_match(dom::Element{"div", {{"class", "myclass"}}}, ".myclass"sv));
        a.expect(is_match(dom::Element{"div", {{"class", "first second"}}}, ".first"sv));
        a.expect(is_match(dom::Element{"div", {{"class", "first second"}}}, ".second"sv));

        a.expect(is_match(dom::Element{"div", {{"class", "first second"}}}, ".first.second"sv));
        a.expect(!is_match(dom::Element{"div", {{"class", "first second"}}}, ".first.third"sv));

        a.expect(is_match(dom::Element{"div", {{"class", "first second"}}}, "div.first"sv));
        a.expect(is_match(dom::Element{"div", {{"class", "first second"}}}, "div.second"sv));
        a.expect(is_match(dom::Element{"div", {{"class", "first second"}}}, "div.second.first"sv));
        a.expect(!is_match(dom::Element{"div", {{"class", "first second"}}}, "div.third"sv));
        a.expect(!is_match(dom::Element{"div", {{"class", "first second"}}}, "p.first"sv));
    });

    s.add_test("is_match: id", [](etest::IActions &a) {
        a.expect(!is_match(dom::Element{"div"}, "#myid"sv));
        a.expect(is_match(dom::Element{"div", {{"class", "myid"}}}, ".myid"sv));
        a.expect(!is_match(dom::Element{"div", {{"id", "myid"}}}, ".myid"sv));
    });

    s.add_test("is_match: psuedo-class, unhandled", [](etest::IActions &a) {
        a.expect(!is_match(dom::Element{"div"}, ":hi"sv));
        a.expect(!is_match(dom::Element{"div"}, "div:hi"sv));
    });

    // These are 100% identical right now as we treat all links as unvisited links.
    for (auto const *pc : std::array{"link", "any-link"}) {
        s.add_test(std::format("is_match: psuedo-class, {}", pc), [pc](etest::IActions &a) {
            a.expect(is_match(dom::Element{"a", {{"href", ""}}}, std::format(":{}", pc)));

            a.expect(is_match(dom::Element{"a", {{"href", ""}}}, std::format("a:{}", pc)));
            a.expect(is_match(dom::Element{"area", {{"href", ""}}}, std::format("area:{}", pc)));

            a.expect(is_match(dom::Element{"a", {{"href", ""}, {"class", "hi"}}}, std::format(".hi:{}", pc)));
            a.expect(is_match(dom::Element{"a", {{"href", ""}, {"id", "hi"}}}, std::format("#hi:{}", pc)));

            a.expect(!is_match(dom::Element{"b"}, std::format(":{}", pc)));
            a.expect(!is_match(dom::Element{"a"}, std::format("a:{}", pc)));
            a.expect(!is_match(dom::Element{"a", {{"href", ""}}}, std::format("b:{}", pc)));
            a.expect(!is_match(dom::Element{"b", {{"href", ""}}}, std::format("b:{}", pc)));
            a.expect(!is_match(dom::Element{"a", {{"href", ""}, {"class", "hi2"}}}, std::format(".hi:{}", pc)));
            a.expect(!is_match(dom::Element{"a", {{"href", ""}, {"id", "hi2"}}}, std::format("#hi:{}", pc)));
        });
    }

    s.add_test("is_match: psuedo-class, :is()", [](etest::IActions &a) {
        a.expect(is_match(dom::Element{"a"}, ":is(a)"sv));
        a.expect(is_match(dom::Element{"a"}, ":is(a, b)"sv));
        a.expect(is_match(dom::Element{"a"}, ":is(b, a)"sv));
        a.expect(!is_match(dom::Element{"b"}, ":is(a)"sv));
        a.expect(!is_match(dom::Element{"c"}, ":is(a, b)"sv));

        // TODO(robinlinden): This should match.
        a.expect(!is_match(dom::Element{"a", {}, {dom::Element{"b"}}}, ":is(a) b"sv));
    });

    s.add_test("is_match: :root", [](etest::IActions &a) {
        dom::Element dom = dom::Element{"html", {}, {dom::Element{"body"}}};
        style::StyledNode node{dom, {}, {style::StyledNode{dom.children[0]}}};
        node.children[0].parent = &node;

        a.expect(style::is_match(node, ":root"));
        a.expect(!style::is_match(node.children[0], ":root"));
    });

    s.add_test("is_match: child", [](etest::IActions &a) {
        dom::Element dom = dom::Element{"div", {{"class", "logo"}}, {dom::Element{"span"}}};
        style::StyledNode node{dom, {}, {style::StyledNode{dom.children[0]}}};
        node.children[0].parent = &node;
        a.expect(style::is_match(node.children[0], ".logo > span"sv));
        a.expect(!style::is_match(node, ".logo > span"sv));

        std::get<dom::Element>(dom.children[0]).attributes["class"] = "ohno";
        a.expect(style::is_match(node.children[0], ".logo > .ohno"sv));
        a.expect(style::is_match(node.children[0], ".logo > span"sv));
    });

    s.add_test("is_match: descendant", [](etest::IActions &a) {
        using style::StyledNode;
        // DOM for div[.logo] { span { a } }
        dom::Element dom = dom::Element{"div", {{"class", "logo"}}, {dom::Element{"span", {}, {dom::Element{"a"}}}}};
        StyledNode node{dom, {}, {{dom.children[0], {}, {{std::get<dom::Element>(dom.children[0]).children[0]}}}}};
        node.children[0].parent = &node;
        node.children[0].children[0].parent = &node.children[0];

        a.expect(style::is_match(node.children[0], ".logo span"sv));
        a.expect(style::is_match(node.children[0], "div span"sv));
        a.expect(!style::is_match(node, ".logo span"sv));

        std::get<dom::Element>(dom.children[0]).attributes["class"] = "ohno";
        a.expect(style::is_match(node.children[0], ".logo .ohno"sv));
        a.expect(style::is_match(node.children[0], ".logo span"sv));

        a.expect(style::is_match(node.children[0].children[0], "div a"sv));
        a.expect(style::is_match(node.children[0].children[0], ".logo a"sv));
        a.expect(style::is_match(node.children[0].children[0], "span a"sv));
        a.expect(style::is_match(node.children[0].children[0], ".ohno a"sv));
        a.expect(style::is_match(node.children[0].children[0], "div span a"sv));
        a.expect(style::is_match(node.children[0].children[0], ".logo span a"sv));
        a.expect(style::is_match(node.children[0].children[0], "div .ohno a"sv));
        a.expect(style::is_match(node.children[0].children[0], ".logo .ohno a"sv));
    });

    s.add_test("matching_rules: simple names", [](etest::IActions &a) {
        css::StyleSheet stylesheet;
        a.expect(matching_rules(dom::Element{"div"}, stylesheet).empty());

        stylesheet.rules.push_back(
                css::Rule{.selectors = {"span", "p"}, .declarations = {{css::PropertyId::Width, "80px"}}});

        a.expect(matching_rules(dom::Element{"div"}, stylesheet).empty());

        {
            auto span_rules = matching_rules(dom::Element{"span"}, stylesheet);
            a.require(span_rules.size() == 1);
            a.expect(span_rules[0] == std::pair{css::PropertyId::Width, "80px"s});
        }

        {
            auto p_rules = matching_rules(dom::Element{"p"}, stylesheet);
            a.require(p_rules.size() == 1);
            a.expect(p_rules[0] == std::pair{css::PropertyId::Width, "80px"s});
        }

        stylesheet.rules.push_back(
                css::Rule{.selectors = {"span", "hr"}, .declarations = {{css::PropertyId::Height, "auto"}}});

        a.expect(matching_rules(dom::Element{"div"}, stylesheet).empty());

        {
            auto span_rules = matching_rules(dom::Element{"span"}, stylesheet);
            a.require(span_rules.size() == 2);
            a.expect(span_rules[0] == std::pair{css::PropertyId::Width, "80px"s});
            a.expect(span_rules[1] == std::pair{css::PropertyId::Height, "auto"s});
        }

        {
            auto p_rules = matching_rules(dom::Element{"p"}, stylesheet);
            a.require(p_rules.size() == 1);
            a.expect(p_rules[0] == std::pair{css::PropertyId::Width, "80px"s});
        }

        {
            auto hr_rules = matching_rules(dom::Element{"hr"}, stylesheet);
            a.require(hr_rules.size() == 1);
            a.expect(hr_rules[0] == std::pair{css::PropertyId::Height, "auto"s});
        }
    });

    s.add_test("matching_rules: media query", [](etest::IActions &a) {
        css::StyleSheet stylesheet{{
                css::Rule{.selectors{"p"}, .declarations{{css::PropertyId::Color, "red"}}},
        }};

        a.expect_eq(
                matching_rules(dom::Element{"p"}, stylesheet), std::vector{std::pair{css::PropertyId::Color, "red"s}});

        stylesheet.rules[0].media_query = css::MediaQuery::parse("(min-width: 700px)");
        a.expect(matching_rules(dom::Element{"p"}, stylesheet).empty());

        a.expect_eq(matching_rules(dom::Element{"p"}, stylesheet, {.window_width = 700}),
                std::vector{std::pair{css::PropertyId::Color, "red"s}});
    });

    s.add_test("style_tree: structure", [](etest::IActions &a) {
        auto root = dom::Element{"html", {}, {}};
        root.children.emplace_back(dom::Element{"head"});
        root.children.emplace_back(dom::Element{"body", {}, {dom::Element{"p"}}});

        style::StyledNode expected{root};
        expected.children.push_back({root.children[0], {}, {}, &expected});
        expected.children.push_back({root.children[1], {}, {}, &expected});

        auto &body = expected.children.back();
        body.children.push_back({std::get<dom::Element>(root.children[1]).children[0], {}, {}, &body});

        a.expect(*style::style_tree(root, {}) == expected);
        a.expect(check_parents(*style::style_tree(root, {}), expected));
    });

    s.add_test("style_tree: style is applied", [](etest::IActions &a) {
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

        a.expect(*style::style_tree(root, stylesheet) == expected);
        a.expect(check_parents(*style::style_tree(root, stylesheet), expected));
    });

    s.add_test("matching rules: custom properties", [](etest::IActions &a) {
        css::StyleSheet stylesheet{{
                css::Rule{.selectors{"p"}, .custom_properties{{"--hello", "very yes"}}},
                css::Rule{.selectors{"a"}, .custom_properties{{"--goodbye", "very no"}}},
        }};

        auto res = style::matching_properties({.node = dom::Element{"p"}}, stylesheet, {});
        a.expect_eq(res.custom, std::vector{std::pair{"--hello"s, "very yes"s}});
        a.expect(res.normal.empty());

        res = style::matching_properties({.node = dom::Element{"a"}}, stylesheet, {});
        a.expect_eq(res.custom, std::vector{std::pair{"--goodbye"s, "very no"s}});
        a.expect(res.normal.empty());

        res = style::matching_properties({.node = dom::Element{"div"}}, stylesheet, {});
        a.expect(res.custom.empty());
        a.expect(res.normal.empty());
    });

    s.add_test("style_tree: custom variables", [](etest::IActions &a) {
        auto root = dom::Element{"html", {{"style", "--inline: 3px;"}}, {}};
        css::StyleSheet stylesheet{{
                {.selectors = {"html"}, .custom_properties = {{"--stylesheet", "5px"}}},
        }};

        style::StyledNode expected{
                .node{root},
                .custom_properties{
                        {"--stylesheet", "5px"},
                        {"--inline", "3px"},
                },
        };

        a.expect_eq(*style::style_tree(root, stylesheet), expected);
    });

    inline_css_tests(s);
    important_declarations_tests(s);
    attribute_selector_matching(s);

    return s.run();
}
