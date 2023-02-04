// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "dom/dom.h"

#include "etest/etest.h"

#include <string_view>
#include <utility>

using namespace std::literals;

using etest::expect;
using etest::expect_eq;
using etest::require;

namespace {
// TODO(robinlinden): Remove.
dom::Node create_element_node(std::string_view name, dom::AttrMap attrs, std::vector<dom::Node> children) {
    return dom::Element{std::string{name}, std::move(attrs), std::move(children)};
}

std::vector<dom::Element const *> nodes_by_xpath(std::reference_wrapper<dom::Node const> root, std::string_view xpath) {
    if (!std::holds_alternative<dom::Element>(root.get())) {
        return {};
    }

    auto const &element = std::get<dom::Element>(root.get());
    return nodes_by_xpath(element, xpath);
}
} // namespace

int main() {
    etest::test("to_string", [] {
        auto document = dom::Document{.doctype{"html5"}};
        document.html_node = dom::Element{.name{"span"}, .children{{dom::Text{"hello"}}}};
        auto expected = "doctype: html5\ntag: span\n  value: hello\n"sv;
        expect_eq(to_string(document), expected);
    });

    etest::test("root not being an element shouldn't crash", [] {
        dom::Node dom = dom::Text{"hello"};
        auto const nodes = nodes_by_xpath(dom, "/anything");
        expect(nodes.empty());
    });

    etest::test("unsupported xpaths don't return anything", [] {
        dom::Node dom = dom::Element{"div"};
        auto nodes = nodes_by_xpath(dom, "div");
        expect(nodes.empty());

        nodes = nodes_by_xpath(dom, "//div");
        expect(nodes.empty());
    });

    // TODO(robinlinden): clang-format doesn't get along well with how I structured
    // the trees in these test cases.
    // clang-format off

    etest::test("no matches", [] {
        auto const dom_root = create_element_node("html", {}, {
            create_element_node("head", {}, {}),
            create_element_node("body", {}, {
                create_element_node("p", {}, {}),
            }),
        });

        auto const nodes = nodes_by_xpath(dom_root, "/html/body/a");
        expect(nodes.empty());
    });

    etest::test("root match", [] {
        auto const dom_root = create_element_node("html", {}, {
            create_element_node("head", {}, {}),
            create_element_node("body", {}, {
                create_element_node("p", {}, {}),
            }),
        });

        auto const nodes = nodes_by_xpath(dom_root, "/html");
        require(nodes.size() == 1);
        expect(nodes[0]->name == "html");
    });

    etest::test("path with one element node", [] {
        auto const dom_root = create_element_node("html", {}, {
            create_element_node("head", {}, {}),
            create_element_node("body", {}, {
                create_element_node("p", {}, {}),
            }),
        });

        auto const nodes = nodes_by_xpath(dom_root, "/html/body/p");
        require(nodes.size() == 1);
        expect(nodes[0]->name == "p");
    });

    etest::test("path with multiple element nodes", [] {
        auto const dom_root = create_element_node("html", {}, {
            create_element_node("head", {}, {}),
            create_element_node("body", {}, {
                create_element_node("p", {}, {}),
                create_element_node("p", {{"display", "none"}}, {}),
            }),
        });

        auto const nodes = nodes_by_xpath(dom_root, "/html/body/p");
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
        auto const dom_root = create_element_node("html", {}, {
            create_element_node("head", {}, {}),
            create_element_node("body", {}, {
                create_element_node("div", {}, {
                    create_element_node("p", {{"display", "none"}}, {}),
                }),
                create_element_node("span", {}, {
                    create_element_node("p", {{"display", "inline"}}, {}),
                }),
                create_element_node("div", {}, {
                    create_element_node("p", {{"display", "block"}}, {}),
                })
            })
        });

        auto const nodes = nodes_by_xpath(dom_root, "/html/body/div/p");
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
        auto const dom_root = create_element_node("html", {}, {
            create_element_node("head", {}, {}),
            dom::Text{"I don't belong here. :("},
            create_element_node("body", {}, {
                create_element_node("p", {}, {}),
            }),
        });

        auto const nodes = nodes_by_xpath(dom_root, "/html/body/p");
        expect(nodes.size() == 1);
    });

    return etest::run_all_tests();
}
