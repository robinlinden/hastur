// SPDX-FileCopyrightText: 2021-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "dom/xpath.h"

#include "dom/dom.h"

#include "etest/etest.h"

#include <string_view>
#include <vector>

using dom::Element;
using dom::Text;
using etest::expect;
using etest::expect_eq;
using etest::require;

namespace {
std::vector<dom::Element const *> nodes_by_xpath(dom::Node const &root, std::string_view xpath) {
    return nodes_by_xpath(std::get<dom::Element>(root), xpath);
}

void descendant_axis_tests() {
    etest::test("descendant axis, root node match", [] {
        dom::Element dom{"div"};
        auto nodes = nodes_by_xpath(dom, "div");
        expect(nodes.empty());

        nodes = nodes_by_xpath(dom, "//div");
        expect_eq(*nodes.at(0), dom);
    });

    etest::test("descendant axis, nested matches", [] {
        dom::Element const &first{"div", {}, {dom::Element{"div", {}, {dom::Element{"div"}}}}};
        auto const &second = std::get<dom::Element>(first.children[0]);
        auto const &third = std::get<dom::Element>(second.children[0]);

        auto nodes = nodes_by_xpath(first, "//div");
        expect_eq(nodes, std::vector{&first, &second, &third});

        nodes = nodes_by_xpath(first, "//div/div");
        expect_eq(nodes, std::vector{&second, &third});

        nodes = nodes_by_xpath(first, "//div//div");
        expect_eq(nodes, std::vector{&second, &third});
    });

    etest::test("descendant axis, no matches", [] {
        dom::Element dom{"div"};
        auto nodes = nodes_by_xpath(dom, "//p");
        expect(nodes.empty());
    });

    etest::test("descendant axis, mixed child and descendant axes", [] {
        dom::Element div{
                .name{"div"},
                .children{
                        dom::Element{"span", {}, {dom::Text{"oh no"}}},
                        dom::Element{"p", {}, {dom::Element{"span", {}, {dom::Element{"a"}}}}},
                        dom::Element{"span"},
                },
        };

        auto const &div_first_span = std::get<dom::Element>(div.children[0]);
        auto const &p = std::get<dom::Element>(div.children[1]);
        auto const &p_span = std::get<dom::Element>(p.children[0]);
        auto const &p_span_a = std::get<dom::Element>(p_span.children[0]);
        auto const &div_last_span = std::get<dom::Element>(div.children[2]);

        auto nodes = nodes_by_xpath(div, "//p");
        expect_eq(nodes, std::vector{&p});

        nodes = nodes_by_xpath(div, "//p/span");
        expect_eq(nodes, std::vector{&p_span});

        nodes = nodes_by_xpath(div, "/div/p//a");
        expect_eq(nodes, std::vector{&p_span_a});

        nodes = nodes_by_xpath(div, "//span");
        expect_eq(nodes, std::vector{&div_first_span, &p_span, &div_last_span});
    });
}

void union_operator_tests() {
    etest::test("union operator", [] {
        dom::Element div{
                .name{"div"},
                .children{
                        dom::Element{"span", {}, {dom::Text{"oh no"}}},
                        dom::Element{"p", {}, {dom::Element{"span", {}, {dom::Element{"a"}}}}},
                        dom::Element{"span"},
                },
        };

        auto const &div_first_span = std::get<dom::Element>(div.children[0]);
        auto const &p = std::get<dom::Element>(div.children[1]);
        auto const &p_span = std::get<dom::Element>(p.children[0]);
        auto const &div_last_span = std::get<dom::Element>(div.children[2]);

        auto nodes = nodes_by_xpath(div, "/div/p|//span");
        expect_eq(nodes, std::vector{&p, &div_first_span, &p_span, &div_last_span});
    });
}

} // namespace

int main() {
    descendant_axis_tests();
    union_operator_tests();

    etest::test("unsupported xpaths don't return anything", [] {
        dom::Node dom = dom::Element{"div"};
        auto nodes = nodes_by_xpath(dom, "div");
        expect(nodes.empty());
    });

    etest::test("no matches", [] {
        auto const dom_root = dom::Element{
                .name{"html"},
                .children{
                        Element{.name{"head"}},
                        Element{.name{"body"}, .children{Element{.name{"p"}}}},
                },
        };

        auto const nodes = nodes_by_xpath(dom_root, "/html/body/a");
        expect(nodes.empty());
    });

    etest::test("root match", [] {
        auto const dom_root = dom::Element{
                .name{"html"},
                .children{
                        Element{.name{"head"}},
                        Element{.name{"body"}, .children{Element{.name{"p"}}}},
                },
        };

        auto const nodes = nodes_by_xpath(dom_root, "/html");
        require(nodes.size() == 1);
        expect(nodes[0]->name == "html");
    });

    etest::test("path with one element node", [] {
        auto const dom_root = dom::Element{
                .name{"html"},
                .children{
                        Element{.name{"head"}},
                        Element{.name{"body"}, .children{Element{.name{"p"}}}},
                },
        };

        auto const nodes = nodes_by_xpath(dom_root, "/html/body/p");
        require(nodes.size() == 1);
        expect(nodes[0]->name == "p");
    });

    etest::test("path with multiple element nodes", [] {
        auto const dom_root = dom::Element{
                .name{"html"},
                .children{
                        Element{.name{"head"}},
                        Element{
                                .name{"body"},
                                .children{
                                        Element{.name{"p"}},
                                        Element{.name{"p"}, .attributes{{"display", "none"}}},
                                },
                        },
                },
        };

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
        auto const dom_root = dom::Element{
                .name{"html"},
                .children{
                        Element{.name{"head"}},
                        Element{
                                .name{"body"},
                                .children{
                                        Element{
                                                .name{"div"},
                                                .children{Element{.name{"p"}, .attributes{{"display", "none"}}}},
                                        },
                                        Element{
                                                .name{"span"},
                                                .children{Element{.name{"p"}, .attributes{{"display", "inline"}}}},
                                        },
                                        Element{
                                                .name{"div"},
                                                .children{Element{.name{"p"}, .attributes{{"display", "block"}}}},
                                        },
                                },
                        },
                },
        };

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
        auto const dom_root = dom::Element{
                .name{"html"},
                .children{
                        Element{.name{"head"}},
                        Text{"I don't belong here. :("},
                        Element{.name{"body"}, .children{Element{.name{"p"}}}},
                },
        };

        auto const nodes = nodes_by_xpath(dom_root, "/html/body/p");
        expect(nodes.size() == 1);
    });

    return etest::run_all_tests();
}
