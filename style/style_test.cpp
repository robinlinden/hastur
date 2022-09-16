// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "style/style.h"
#include "style/styled_node.h"

#include "css/rule.h"
#include "etest/etest.h"

#include <fmt/format.h>

#include <algorithm>
#include <array>

using namespace std::literals;
using etest::expect;
using etest::require;

namespace {
bool check_parents(style::StyledNode const &a, style::StyledNode const &b) {
    if (!std::equal(cbegin(a.children), cend(a.children), cbegin(b.children), cend(b.children), &check_parents)) {
        return false;
    }

    if (a.parent == nullptr || b.parent == nullptr) {
        return a.parent == b.parent;
    }

    return *a.parent == *b.parent;
}
} // namespace

int main() {
    etest::test("is_match: simple names", [] {
        expect(style::is_match(dom::Element{"div"}, "div"sv));
        expect(!style::is_match(dom::Element{"div"}, "span"sv));
    });

    etest::test("is_match: class", [] {
        expect(!style::is_match(dom::Element{"div"}, ".myclass"sv));
        expect(!style::is_match(dom::Element{"div", {{"id", "myclass"}}}, ".myclass"sv));
        expect(style::is_match(dom::Element{"div", {{"class", "myclass"}}}, ".myclass"sv));
    });

    etest::test("is_match: id", [] {
        expect(!style::is_match(dom::Element{"div"}, "#myid"sv));
        expect(style::is_match(dom::Element{"div", {{"class", "myid"}}}, ".myid"sv));
        expect(!style::is_match(dom::Element{"div", {{"id", "myid"}}}, ".myid"sv));
    });

    etest::test("is_match: psuedo-class, unhandled", [] {
        expect(!style::is_match(dom::Element{"div"}, ":hi"sv));
        expect(!style::is_match(dom::Element{"div"}, "div:hi"sv));
    });

    // These are 100% identical right now as we treat all links as unvisited links.
    for (auto const pc : std::array{"link", "any-link"}) {
        etest::test(fmt::format("is_match: psuedo-class, {}", pc), [pc] {
            expect(style::is_match(dom::Element{"a", {{"href", ""}}}, fmt::format(":{}", pc)));

            expect(style::is_match(dom::Element{"a", {{"href", ""}}}, fmt::format("a:{}", pc)));
            expect(style::is_match(dom::Element{"area", {{"href", ""}}}, fmt::format("area:{}", pc)));

            expect(style::is_match(dom::Element{"a", {{"href", ""}, {"class", "hi"}}}, fmt::format(".hi:{}", pc)));
            expect(style::is_match(dom::Element{"a", {{"href", ""}, {"id", "hi"}}}, fmt::format("#hi:{}", pc)));

            expect(!style::is_match(dom::Element{"b"}, fmt::format(":{}", pc)));
            expect(!style::is_match(dom::Element{"a"}, fmt::format("a:{}", pc)));
            expect(!style::is_match(dom::Element{"a", {{"href", ""}}}, fmt::format("b:{}", pc)));
            expect(!style::is_match(dom::Element{"b", {{"href", ""}}}, fmt::format("b:{}", pc)));
            expect(!style::is_match(dom::Element{"a", {{"href", ""}, {"class", "hi2"}}}, fmt::format(".hi:{}", pc)));
            expect(!style::is_match(dom::Element{"a", {{"href", ""}, {"id", "hi2"}}}, fmt::format("#hi:{}", pc)));
        });
    }

    etest::test("matching_rules: simple names", [] {
        std::vector<css::Rule> stylesheet;
        expect(style::matching_rules(dom::Element{"div"}, stylesheet).empty());

        stylesheet.push_back(css::Rule{.selectors = {"span", "p"}, .declarations = {{"width", "80px"}}});

        expect(style::matching_rules(dom::Element{"div"}, stylesheet).empty());

        {
            auto span_rules = style::matching_rules(dom::Element{"span"}, stylesheet);
            require(span_rules.size() == 1);
            expect(span_rules[0] == std::pair{"width"s, "80px"s});
        }

        {
            auto p_rules = style::matching_rules(dom::Element{"p"}, stylesheet);
            require(p_rules.size() == 1);
            expect(p_rules[0] == std::pair{"width"s, "80px"s});
        }

        stylesheet.push_back(css::Rule{.selectors = {"span", "hr"}, .declarations = {{"height", "auto"}}});

        expect(style::matching_rules(dom::Element{"div"}, stylesheet).empty());

        {
            auto span_rules = style::matching_rules(dom::Element{"span"}, stylesheet);
            require(span_rules.size() == 2);
            expect(span_rules[0] == std::pair{"width"s, "80px"s});
            expect(span_rules[1] == std::pair{"height"s, "auto"s});
        }

        {
            auto p_rules = style::matching_rules(dom::Element{"p"}, stylesheet);
            require(p_rules.size() == 1);
            expect(p_rules[0] == std::pair{"width"s, "80px"s});
        }

        {
            auto hr_rules = style::matching_rules(dom::Element{"hr"}, stylesheet);
            require(hr_rules.size() == 1);
            expect(hr_rules[0] == std::pair{"height"s, "auto"s});
        }
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

        std::vector<css::Rule> stylesheet{
                {.selectors = {"p"}, .declarations = {{"height", "100px"}}},
                {.selectors = {"body"}, .declarations = {{"text-size", "500em"}}},
        };

        style::StyledNode expected{root};
        expected.children.push_back({root.children[0], {}, {}, &expected});
        expected.children.push_back({root.children[1], {{"text-size", "500em"}}, {}, &expected});
        auto &body = expected.children.back();

        body.children.push_back(
                {std::get<dom::Element>(root.children[1]).children[0], {{"height", "100px"}}, {}, &body});

        expect(*style::style_tree(root, stylesheet) == expected);
        expect(check_parents(*style::style_tree(root, stylesheet), expected));
    });

    return etest::run_all_tests();
}
