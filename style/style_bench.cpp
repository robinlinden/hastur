// SPDX-FileCopyrightText: 2025-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "style/style.h"

#include "style/styled_node.h"

#include "dom/dom.h"
#include "etest/etest2.h"

#include <vector>

namespace {
void set_up_parent_ptrs(style::StyledNode &root) {
    std::vector<style::StyledNode *> stack{&root};
    while (!stack.empty()) {
        auto *current = stack.back();
        stack.pop_back();

        for (auto &child : current->children) {
            child.parent = current;
            stack.push_back(&child);
        }
    }
}
} // namespace

int main() {
    etest::Suite s;

    dom::Node few_classes_dom = dom::Element{"div", {{"class", "first second"}}};
    auto few_classes = style::StyledNode{.node = few_classes_dom};

    s.add_benchmark("is_match: class, few classes", [&] {
        style::is_match(few_classes, ".first.second"); //
    });

    s.add_benchmark("no match, few classes", [&] {
        style::is_match(few_classes, ".first.second.third.fourth"); //
    });

    dom::Node many_classes_dom = dom::Element{
            "div",
            {{"class", "one two three four five six seven eight nine ten"}},
    };
    auto many_classes = style::StyledNode{.node = many_classes_dom};

    s.add_benchmark("match, many classes", [&] {
        style::is_match(many_classes, ".eight.two.seven.ten"); //
    });

    s.add_benchmark("no match, many classes", [&] {
        style::is_match(many_classes, ".eight.two.seve.ten"); //
    });

    dom::Node shallow_dom = dom::Element{"div", {}, {dom::Element{"span"}}};
    auto shallow = style::StyledNode{
            .node = shallow_dom,
            .children = {{std::get<dom::Element>(shallow_dom).children.back()}},
    };
    set_up_parent_ptrs(shallow);

    s.add_benchmark("match, shallow", [&] {
        style::is_match(shallow.children.back(), "div span"); //
    });

    s.add_benchmark("no match, shallow", [&] {
        style::is_match(shallow.children.back(), "div span div"); //
    });

    dom::Node deep_dom = dom::Element{"div"};
    style::StyledNode deep{.node = deep_dom};
    {
        // Since StyledNode only holds a reference to the dom node, we can
        // reuse this one node and just make the style tree very deep.
        auto *current = &deep;
        for (int i = 0; i < 16; ++i) {
            current = &current->children.emplace_back(deep_dom);
        }
        set_up_parent_ptrs(deep);
    }

    s.add_benchmark("no match, 4 selectors, shallowest", [&] {
        style::is_match(deep, "div div div div"); //
    });

    auto const *deepest_node = &deep;
    while (!deepest_node->children.empty()) {
        deepest_node = &deepest_node->children.back();
    }

    s.add_benchmark("match, 4 selectors, deepest", [&] {
        style::is_match(*deepest_node, "div div div div"); //
    });

    s.add_benchmark("match, 8 selectors, deepest", [&] {
        style::is_match(*deepest_node, "div div div div div div div div"); //
    });

    s.add_benchmark("no match, 8 selectors, deepest", [&] {
        style::is_match(*deepest_node, "p div div div div div div div"); //
    });

    return s.run();
}
