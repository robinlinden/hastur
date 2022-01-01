// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "html/parse.h"

#include "etest/etest.h"

using namespace std::literals;
using etest::expect;
using etest::require;

int main() {
    etest::test("doctype", [] {
        auto document = html::parse("<!doctype html>"sv);
        expect(document.html().children.size() == 0);
        expect(document.doctype == "html"s);
    });

    etest::test("weirdly capitalized doctype", [] {
        auto document = html::parse("<!docTYpe html>"sv);
        expect(document.html().children.size() == 0);
        expect(document.doctype == "html"s);
    });

    etest::test("everything is wrapped in a html element", [] {
        auto document = html::parse("<p></p>"sv);
        auto html = document.html();
        expect(html.name == "html"s);
        require(html.children.size() == 1);
        expect(std::get<dom::Element>(html.children[0]).name == "p"s);
    });

    etest::test("single element", [] {
        auto html = html::parse("<html></html>"sv).html();
        expect(html.children.size() == 0);
        expect(html.name == "html"s);
        expect(html.attributes.size() == 0);
    });

    etest::test("self-closing single element", [] {
        auto nodes = html::parse("<br>"sv).html().children;
        require(nodes.size() == 1);

        auto br = std::get<dom::Element>(nodes[0]);
        expect(br.children.size() == 0);
        expect(br.name == "br"s);
        expect(br.attributes.size() == 0);
    });

    etest::test("self-closing single element with slash", [] {
        auto nodes = html::parse("<img/>"sv).html().children;
        require(nodes.size() == 1);

        auto img = std::get<dom::Element>(nodes[0]);
        expect(img.children.size() == 0);
        expect(img.name == "img"s);
        expect(img.attributes.size() == 0);
    });

    etest::test("multiple elements", [] {
        auto nodes = html::parse("<span></span><div></div>"sv).html().children;
        require(nodes.size() == 2);

        auto span = std::get<dom::Element>(nodes[0]);
        expect(span.children.size() == 0);
        expect(span.name == "span"s);
        expect(span.attributes.size() == 0);

        auto div = std::get<dom::Element>(nodes[1]);
        expect(div.children.size() == 0);
        expect(div.name == "div"s);
        expect(div.attributes.size() == 0);
    });

    etest::test("nested elements", [] {
        auto html = html::parse("<html><body></body></html>"sv).html();
        require(html.children.size() == 1);
        expect(html.name == "html"s);
        expect(html.attributes.size() == 0);

        auto body = std::get<dom::Element>(html.children[0]);
        expect(body.name == "body"s);
        expect(body.attributes.size() == 0);
    });

    etest::test("single-quoted attribute", [] {
        auto nodes = html::parse("<meta charset='utf-8'/>"sv).html().children;
        require(nodes.size() == 1);

        auto meta = std::get<dom::Element>(nodes[0]);
        expect(meta.children.size() == 0);
        expect(meta.name == "meta"s);
        expect(meta.attributes.size() == 1);
        expect(meta.attributes.at("charset") == "utf-8"s);
    });

    etest::test("double-quoted attribute", [] {
        auto nodes = html::parse(R"(<meta charset="utf-8"/>)"sv).html().children;
        require(nodes.size() == 1);

        auto meta = std::get<dom::Element>(nodes[0]);
        expect(meta.children.size() == 0);
        expect(meta.name == "meta"s);
        expect(meta.attributes.size() == 1);
        expect(meta.attributes.at("charset"s) == "utf-8"s);
    });

    etest::test("multiple attributes", [] {
        auto nodes = html::parse(R"(<meta name="viewport" content="width=100em, initial-scale=1"/>)"sv).html().children;
        require(nodes.size() == 1);

        auto meta = std::get<dom::Element>(nodes[0]);
        expect(meta.children.size() == 0);
        expect(meta.name == "meta"s);
        expect(meta.attributes.size() == 2);
        expect(meta.attributes.at("name"s) == "viewport"s);
        expect(meta.attributes.at("content"s) == "width=100em, initial-scale=1"s);
    });

    etest::test("multiple nodes with attributes", [] {
        auto html = html::parse("<html bonus='hello'><body style='fancy'></body></html>"sv).html();
        require(html.children.size() == 1);
        expect(html.name == "html"s);
        expect(html.attributes.size() == 1);
        expect(html.attributes.at("bonus"s) == "hello"s);

        auto body = std::get<dom::Element>(html.children[0]);
        expect(body.name == "body"s);
        expect(body.attributes.size() == 1);
        expect(body.attributes.at("style"s) == "fancy"s);
    });

    etest::test("text node", [] {
        auto html = html::parse("<html>fantastic, the future is now</html>"sv).html();
        require(html.children.size() == 1);
        expect(html.name == "html"s);
        expect(html.attributes.size() == 0);

        auto text = std::get<dom::Text>(html.children[0]);
        expect(text.text == "fantastic, the future is now"s);
    });

    etest::test("character reference in attribute", [] {
        auto html = html::parse("<html test='&lt;3'></html>"sv).html();
        expect(html.children.size() == 0);
        expect(html.name == "html"s);
        expect(html.attributes.size() == 1);
        expect(html.attributes.at("test") == "<3");
    });

    etest::test("character reference in attribute, no semicolon", [] {
        auto html = html::parse("<html test='&lt3'></html>"sv).html();
        expect(html.children.size() == 0);
        expect(html.name == "html"s);
        expect(html.attributes.size() == 1);
        expect(html.attributes.at("test") == "&lt3");
    });

    return etest::run_all_tests();
}
