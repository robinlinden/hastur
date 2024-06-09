// SPDX-FileCopyrightText: 2021-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "dom/xpath.h"

#include "dom/dom.h"

#include "etest/etest2.h"

#include <string_view>
#include <vector>

using dom::Element;
using dom::Text;

namespace {
std::vector<dom::Element const *> nodes_by_xpath(dom::Node const &root, std::string_view xpath) {
    return nodes_by_xpath(std::get<dom::Element>(root), xpath);
}

void descendant_axis_tests(etest::Suite &s) {
    s.add_test("descendant axis, root node match", [](etest::IActions &a) {
        dom::Element dom{"div"};
        auto nodes = nodes_by_xpath(dom, "div");
        a.expect(nodes.empty());

        nodes = nodes_by_xpath(dom, "//div");
        a.expect_eq(*nodes.at(0), dom);
    });

    s.add_test("descendant axis, nested matches", [](etest::IActions &a) {
        dom::Element const &first{"div", {}, {dom::Element{"div", {}, {dom::Element{"div"}}}}};
        auto const &second = std::get<dom::Element>(first.children[0]);
        auto const &third = std::get<dom::Element>(second.children[0]);

        auto nodes = nodes_by_xpath(first, "//div");
        a.expect_eq(nodes, std::vector{&first, &second, &third});

        nodes = nodes_by_xpath(first, "//div/div");
        a.expect_eq(nodes, std::vector{&second, &third});

        nodes = nodes_by_xpath(first, "//div//div");
        a.expect_eq(nodes, std::vector{&second, &third});
    });

    s.add_test("descendant axis, no matches", [](etest::IActions &a) {
        dom::Element dom{"div"};
        auto nodes = nodes_by_xpath(dom, "//p");
        a.expect(nodes.empty());
    });

    s.add_test("descendant axis, mixed child and descendant axes", [](etest::IActions &a) {
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
        a.expect_eq(nodes, std::vector{&p});

        nodes = nodes_by_xpath(div, "//p/span");
        a.expect_eq(nodes, std::vector{&p_span});

        nodes = nodes_by_xpath(div, "/div/p//a");
        a.expect_eq(nodes, std::vector{&p_span_a});

        nodes = nodes_by_xpath(div, "//span");
        a.expect_eq(nodes, std::vector{&div_first_span, &p_span, &div_last_span});
    });
}

void union_operator_tests(etest::Suite &s) {
    s.add_test("union operator", [](etest::IActions &a) {
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
        a.expect_eq(nodes, std::vector{&p, &div_first_span, &p_span, &div_last_span});
    });
}

} // namespace

int main() {
    etest::Suite s{"xpath"};

    descendant_axis_tests(s);
    union_operator_tests(s);

    s.add_test("unsupported xpaths don't return anything", [](etest::IActions &a) {
        dom::Node dom = dom::Element{"div"};
        auto nodes = nodes_by_xpath(dom, "div");
        a.expect(nodes.empty());
    });

    s.add_test("no matches", [](etest::IActions &a) {
        auto const dom_root = dom::Element{
                .name{"html"},
                .children{
                        Element{.name{"head"}},
                        Element{.name{"body"}, .children{Element{.name{"p"}}}},
                },
        };

        auto const nodes = nodes_by_xpath(dom_root, "/html/body/a");
        a.expect(nodes.empty());
    });

    s.add_test("root match", [](etest::IActions &a) {
        auto const dom_root = dom::Element{
                .name{"html"},
                .children{
                        Element{.name{"head"}},
                        Element{.name{"body"}, .children{Element{.name{"p"}}}},
                },
        };

        auto const nodes = nodes_by_xpath(dom_root, "/html");
        a.require(nodes.size() == 1);
        a.expect(nodes[0]->name == "html");
    });

    s.add_test("path with one element node", [](etest::IActions &a) {
        auto const dom_root = dom::Element{
                .name{"html"},
                .children{
                        Element{.name{"head"}},
                        Element{.name{"body"}, .children{Element{.name{"p"}}}},
                },
        };

        auto const nodes = nodes_by_xpath(dom_root, "/html/body/p");
        a.require(nodes.size() == 1);
        a.expect(nodes[0]->name == "p");
    });

    s.add_test("path with multiple element nodes", [](etest::IActions &a) {
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
        a.require(nodes.size() == 2);

        auto const first = *nodes[0];
        a.expect(first.name == "p");
        a.expect(first.attributes.empty());

        auto const second = *nodes[1];
        a.expect(second.name == "p");
        a.expect(second.attributes.size() == 1);
        a.expect(second.attributes.at("display") == "none");
    });

    s.add_test("matching nodes in different branches", [](etest::IActions &a) {
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
        a.require(nodes.size() == 2);

        auto const first = *nodes[0];
        a.expect(first.name == "p");
        a.expect(first.attributes.size() == 1);
        a.expect(first.attributes.at("display") == "none");

        auto const second = *nodes[1];
        a.expect(second.name == "p");
        a.expect(second.attributes.size() == 1);
        a.expect(second.attributes.at("display") == "block");
    });

    s.add_test("non-element node in search path", [](etest::IActions &a) {
        auto const dom_root = dom::Element{
                .name{"html"},
                .children{
                        Element{.name{"head"}},
                        Text{"I don't belong here. :("},
                        Element{.name{"body"}, .children{Element{.name{"p"}}}},
                },
        };

        auto const nodes = nodes_by_xpath(dom_root, "/html/body/p");
        a.expect(nodes.size() == 1);
    });

    return s.run();
}
