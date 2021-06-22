#include "dom/dom.h"

#include "etest/etest.h"

using etest::expect;
using etest::require;

int main() {
    etest::test("no matches", [] {
        auto const dom_root = dom::create_element_node("html", {}, {
            dom::create_element_node("head", {}, {}),
            dom::create_element_node("body", {}, {
                dom::create_element_node("p", {}, {}),
            }),
        });

        auto const nodes = nodes_by_path(dom_root, "html.body.a");
        expect(nodes.size() == 0);
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
        expect(std::get<dom::Element>(nodes[0]->data).name == "html");
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
        expect(std::get<dom::Element>(nodes[0]->data).name == "p");
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

        auto const first = std::get<dom::Element>(nodes[0]->data);
        expect(first.name == "p");
        expect(first.attributes.size() == 0);

        auto const second = std::get<dom::Element>(nodes[1]->data);
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

        auto const first = std::get<dom::Element>(nodes[0]->data);
        expect(first.name == "p");
        expect(first.attributes.size() == 1);
        expect(first.attributes.at("display") == "none");

        auto const second = std::get<dom::Element>(nodes[1]->data);
        expect(second.name == "p");
        expect(second.attributes.size() == 1);
        expect(second.attributes.at("display") == "block");
    });

    return etest::run_all_tests();
}
