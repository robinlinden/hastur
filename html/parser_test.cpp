// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "html/parser.h"

#include "etest/etest.h"

#include <cstddef>

using namespace std::literals;
using etest::expect;
using etest::expect_eq;
using etest::require;
using etest::require_eq;

namespace {
dom::Element const &body(dom::Document const &d) {
    return std::get<dom::Element>(d.html().children.at(1));
}
} // namespace

int main() {
    etest::test("doctype", [] {
        auto document = html::parse("<!doctype html>"sv);
        expect(document.doctype == "html"s);
    });

    etest::test("weirdly capitalized doctype", [] {
        auto document = html::parse("<!docTYpe html>"sv);
        expect(document.doctype == "html"s);
    });

    etest::test("everything is wrapped in a html element", [] {
        auto document = html::parse("<p></p>"sv);
        auto html = document.html();
        expect(html.name == "html"s);
        require(html.children.size() == 2);
        expect(std::get<dom::Element>(html.children[0]).name == "head"s);

        auto const &body = std::get<dom::Element>(html.children[1]);
        expect_eq(body.name, "body"s);

        expect_eq(std::get<dom::Element>(body.children.at(0)).name, "p");
    });

    etest::test("single element", [] {
        auto html = html::parse("<html></html>"sv).html();
        expect(html.name == "html"s);
        expect(html.attributes.empty());
    });

    etest::test("self-closing single element", [] {
        auto doc = html::parse("<br>"sv);

        auto const &br = std::get<dom::Element>(body(doc).children.at(0));
        expect(br.children.empty());
        expect(br.name == "br"s);
        expect(br.attributes.empty());
    });

    etest::test("self-closing single element with slash", [] {
        auto doc = html::parse("<img/>"sv);

        auto const &img = std::get<dom::Element>(body(doc).children.at(0));
        expect(img.children.empty());
        expect(img.name == "img"s);
        expect(img.attributes.empty());
    });

    etest::test("multiple elements", [] {
        auto doc = html::parse("<span></span><div></div>"sv);

        auto const &span = std::get<dom::Element>(body(doc).children[0]);
        expect(span.children.empty());
        expect(span.name == "span"s);
        expect(span.attributes.empty());

        auto const &div = std::get<dom::Element>(body(doc).children[1]);
        expect(div.children.empty());
        expect(div.name == "div"s);
        expect(div.attributes.empty());
    });

    etest::test("nested elements", [] {
        auto html = html::parse("<html><body></body></html>"sv).html();
        require(html.children.size() == 2);
        expect(html.name == "html"s);
        expect(html.attributes.empty());

        auto const &head = std::get<dom::Element>(html.children[0]);
        expect(head.name == "head"s);
        expect(head.attributes.empty());

        auto const &body = std::get<dom::Element>(html.children[1]);
        expect(body.name == "body"s);
        expect(body.attributes.empty());
    });

    etest::test("single-quoted attribute", [] {
        auto nodes = html::parse("<meta charset='utf-8'/>"sv).html().children;
        require(nodes.size() == 2);

        auto const &head = std::get<dom::Element>(nodes[0]);
        auto const &meta = std::get<dom::Element>(head.children.at(0));
        expect(meta.children.empty());
        expect(meta.name == "meta"s);
        expect(meta.attributes.size() == 1);
        expect(meta.attributes.at("charset") == "utf-8"s);
    });

    etest::test("double-quoted attribute", [] {
        auto nodes = html::parse(R"(<meta charset="utf-8"/>)"sv).html().children;
        require(nodes.size() == 2);

        auto const &head = std::get<dom::Element>(nodes[0]);
        auto const &meta = std::get<dom::Element>(head.children.at(0));
        expect(meta.children.empty());
        expect(meta.name == "meta"s);
        expect(meta.attributes.size() == 1);
        expect(meta.attributes.at("charset"s) == "utf-8"s);
    });

    etest::test("multiple attributes", [] {
        auto nodes = html::parse(R"(<meta name="viewport" content="width=100em, initial-scale=1"/>)"sv).html().children;
        require(nodes.size() == 2);

        auto const &head = std::get<dom::Element>(nodes[0]);
        auto const &meta = std::get<dom::Element>(head.children.at(0));
        expect(meta.children.empty());
        expect(meta.name == "meta"s);
        expect(meta.attributes.size() == 2);
        expect(meta.attributes.at("name"s) == "viewport"s);
        expect(meta.attributes.at("content"s) == "width=100em, initial-scale=1"s);
    });

    etest::test("multiple nodes with attributes", [] {
        auto html = html::parse("<html bonus='hello'><body style='fancy'></body></html>"sv).html();
        require(html.children.size() == 2);
        expect(html.name == "html"s);
        expect(html.attributes.size() == 1);
        expect(html.attributes.at("bonus"s) == "hello"s);

        auto const &body = std::get<dom::Element>(html.children[1]);
        expect(body.name == "body"s);
        expect(body.attributes.size() == 1);
        expect(body.attributes.at("style"s) == "fancy"s);
    });

    etest::test("text node", [] {
        auto html = html::parse("<html>fantastic, the future is now</html>"sv).html();
        expect_eq(html.children.size(), std::size_t{2});

        auto const &head = std::get<dom::Element>(html.children.at(0));
        expect_eq(head, dom::Element{.name = "head"});
        auto const &body = std::get<dom::Element>(html.children.at(1));
        expect_eq(body, dom::Element{.name = "body", .children{dom::Text{"fantastic, the future is now"}}});
    });

    etest::test("character reference in attribute", [] {
        auto html = html::parse("<html test='&lt;3'></html>"sv).html();
        expect(html.name == "html"s);
        expect(html.attributes.size() == 1);
        expect(html.attributes.at("test") == "<3");
    });

    etest::test("character reference in attribute, no semicolon", [] {
        auto html = html::parse("<html test='&lt3'></html>"sv).html();
        expect(html.name == "html"s);
        expect(html.attributes.size() == 1);
        expect(html.attributes.at("test") == "&lt3");
    });

    etest::test("br shouldn't open a new scope", [] {
        auto html = html::parse("<br><p></p>"sv).html();
        require(html.children.size() == 2);

        auto const &body = std::get<dom::Element>(html.children[1]);

        auto const &br = std::get<dom::Element>(body.children[0]);
        expect(br.name == "br"sv);
        expect(br.children.empty());

        auto const &p = std::get<dom::Element>(body.children[1]);
        expect(p.name == "p"sv);
        expect(p.children.empty());
    });

    etest::test("script is handled correctly", [] {
        auto html = html::parse("<script><hello></script>"sv).html();
        require_eq(html.children.size(), std::size_t{2});

        auto const &head = std::get<dom::Element>(html.children[0]);
        auto const &script = std::get<dom::Element>(head.children.at(0));
        expect_eq(script.name, "script"sv);
        expect_eq(script.children.size(), std::size_t{1});

        auto const &script_content = std::get<dom::Text>(script.children[0]);
        expect_eq(script_content.text, "<hello>"sv);
    });

    etest::test("special rules, p end tag omission", [] {
        auto doc = html::parse("<html><p>hello<p>world</html>"sv);

        auto const &p1 = std::get<dom::Element>(body(doc).children.at(0));
        expect_eq(p1.name, "p");

        require_eq(p1.children.size(), std::size_t{1});
        auto const &p1_text = std::get<dom::Text>(p1.children[0]);
        expect_eq(p1_text, dom::Text{"hello"});

        auto const &p2 = std::get<dom::Element>(body(doc).children[1]);
        expect_eq(p2.name, "p");

        require_eq(p2.children.size(), std::size_t{1});
        auto const &p2_text = std::get<dom::Text>(p2.children[0]);
        expect_eq(p2_text, dom::Text{"world"});
    });

    etest::test("special rules, html tag omission", [] {
        auto html = html::parse("<head></head><body>hello</body>"sv).html();
        require_eq(html.children.size(), std::size_t{2});

        auto const &head = std::get<dom::Element>(html.children[0]);
        expect_eq(head.name, "head");

        auto const &body = std::get<dom::Element>(html.children[1]);
        expect_eq(body.name, "body");

        require_eq(body.children.size(), std::size_t{1});
        auto const &body_text = std::get<dom::Text>(body.children[0]);
        expect_eq(body_text, dom::Text{"hello"});
    });

    etest::test("special rules, an empty string still parses as html", [] {
        auto html = html::parse("").html();
        require_eq(html.children.size(), std::size_t{2});

        auto const &head = std::get<dom::Element>(html.children[0]);
        expect_eq(head.name, "head");
        auto const &body = std::get<dom::Element>(html.children[1]);
        expect_eq(body.name, "body");
    });

    etest::test("special rules, just closing html is fine", [] {
        auto html = html::parse("</html>").html();
        require_eq(html.children.size(), std::size_t{2});

        auto const &head = std::get<dom::Element>(html.children[0]);
        expect_eq(head.name, "head");
        auto const &body = std::get<dom::Element>(html.children[1]);
        expect_eq(body.name, "body");
    });

    etest::test("special rules, head is auto-closed on html end tag", [] {
        auto html = html::parse("<head></html>").html();
        require_eq(html.children.size(), std::size_t{2});

        auto const &head = std::get<dom::Element>(html.children[0]);
        expect_eq(head.name, "head");
    });

    etest::test("special rules, head tag omission", [] {
        auto html = html::parse("<title>hello</title>"sv).html();

        auto const &head = std::get<dom::Element>(html.children.at(0));
        expect_eq(head.name, "head");

        auto const &title = std::get<dom::Element>(head.children.at(0));
        expect_eq(title.name, "title");
    });

    etest::test("special rules, just text is fine too", [] {
        auto html = html::parse("hello?"sv).html();
        require_eq(html.children.size(), std::size_t{2});

        auto const &head = std::get<dom::Element>(html.children[0]);
        expect_eq(head.name, "head");

        auto const &body = std::get<dom::Element>(html.children[1]);
        expect_eq(body.name, "body");

        require_eq(body.children.size(), std::size_t{1});
        auto const &body_text = std::get<dom::Text>(body.children[0]);
        expect_eq(body_text, dom::Text{"hello?"});
    });

    etest::test("<style> consumes everything as raw text", [] {
        auto html = html::parse("<style><body>"sv).html();

        auto const &head = std::get<dom::Element>(html.children.at(0));
        expect_eq(head.name, "head");

        auto const &style = std::get<dom::Element>(head.children.at(0));
        expect_eq(style.name, "style");
        expect_eq(style.children.size(), std::size_t{1});

        auto const &text = std::get<dom::Text>(style.children.at(0));
        expect_eq(text.text, "<body>");
    });

    etest::test("<noscript> w/ scripting consumes everything as raw text", [] {
        auto html = html::parse("<body><noscript><span>"sv, {.scripting = true}).html();

        auto const &body = std::get<dom::Element>(html.children.at(1));
        expect_eq(body.name, "body");

        auto const &noscript = std::get<dom::Element>(body.children.at(0));
        expect_eq(noscript.name, "noscript");
        expect_eq(noscript.children.size(), std::size_t{1});

        auto const &text = std::get<dom::Text>(noscript.children.at(0));
        expect_eq(text.text, "<span>");
    });

    etest::test("<noscript> w/o scripting is treated as a normal dom node", [] {
        auto html = html::parse("<body><noscript><span>"sv, {.scripting = false}).html();

        auto const &body = std::get<dom::Element>(html.children.at(1));
        expect_eq(body.name, "body");

        auto const &noscript = std::get<dom::Element>(body.children.at(0));
        expect_eq(noscript.name, "noscript");
        expect_eq(noscript.children.size(), std::size_t{1});

        auto const &span = std::get<dom::Element>(noscript.children.at(0));
        expect_eq(span.name, "span");
    });

    return etest::run_all_tests();
}
