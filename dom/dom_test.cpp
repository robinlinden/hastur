// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "dom/dom.h"

#include "etest/etest.h"

using etest::expect;
using etest::require;

// TODO(robinlinden): clang-format doesn't get along well with how I structured
// the trees in these test cases.

// clang-format off

int main() {
    etest::test("no matches", [] {
        auto const dom_root = dom::create_element_node("html", {}, {
            dom::create_element_node("head", {}, {}),
            dom::create_element_node("body", {}, {
                dom::create_element_node("p", {}, {}),
            }),
        });

        auto const nodes = nodes_by_path(dom_root, "html.body.a");
        expect(nodes.empty());
    });

    etest::test("root match", [] {
        auto const dom_root = dom::create_element_node("html", {}, {
            dom::create_element_node("head", {}, {}),
            dom::create_element_node("body", {}, {
                dom::create_element_node("p", {}, {}),
            }),
        });

        auto const nodes = nodes_by_path(dom_root, "html");
        require(nodes.size() == 1);
        expect(nodes[0]->name == "html");
    });

    etest::test("path with one element node", [] {
        auto const dom_root = dom::create_element_node("html", {}, {
            dom::create_element_node("head", {}, {}),
            dom::create_element_node("body", {}, {
                dom::create_element_node("p", {}, {}),
            }),
        });

        auto const nodes = nodes_by_path(dom_root, "html.body.p");
        require(nodes.size() == 1);
        expect(nodes[0]->name == "p");
    });

    etest::test("path with multiple element nodes", [] {
        auto const dom_root = dom::create_element_node("html", {}, {
            dom::create_element_node("head", {}, {}),
            dom::create_element_node("body", {}, {
                dom::create_element_node("p", {}, {}),
                dom::create_element_node("p", {{"display", "none"}}, {}),
            }),
        });

        auto const nodes = nodes_by_path(dom_root, "html.body.p");
        require(nodes.size() == 2);

        auto const first = *nodes[0];
        expect(first.name == "p");
        expect(first.attributes.empty());

        auto const second = *nodes[1];
        expect(second.name == "p");
        expect(second.attributes.size() == 1);
        expect(second.attributes.at("display") == "none");
    });

    etest::test("matching nodes in different branches", [] {
        auto const dom_root = dom::create_element_node("html", {}, {
            dom::create_element_node("head", {}, {}),
            dom::create_element_node("body", {}, {
                dom::create_element_node("div", {}, {
                    dom::create_element_node("p", {{"display", "none"}}, {}),
                }),
                dom::create_element_node("span", {}, {
                    dom::create_element_node("p", {{"display", "inline"}}, {}),
                }),
                dom::create_element_node("div", {}, {
                    dom::create_element_node("p", {{"display", "block"}}, {}),
                })
            })
        });

        auto const nodes = nodes_by_path(dom_root, "html.body.div.p");
        require(nodes.size() == 2);

        auto const first = *nodes[0];
        expect(first.name == "p");
        expect(first.attributes.size() == 1);
        expect(first.attributes.at("display") == "none");

        auto const second = *nodes[1];
        expect(second.name == "p");
        expect(second.attributes.size() == 1);
        expect(second.attributes.at("display") == "block");
    });

    etest::test("non-element node in search path", [] {
        auto const dom_root = dom::create_element_node("html", {}, {
            dom::create_element_node("head", {}, {}),
            dom::Text{"I don't belong here. :("},
            dom::create_element_node("body", {}, {
                dom::create_element_node("p", {}, {}),
            }),
        });

        auto const nodes = nodes_by_path(dom_root, "html.body.p");
        expect(nodes.size() == 1);
    });

    return etest::run_all_tests();
}
