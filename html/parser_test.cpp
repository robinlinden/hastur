// SPDX-FileCopyrightText: 2021-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "html/parser.h"

#include "dom/dom.h"
#include "etest/etest2.h"
#include "html2/parse_error.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

using namespace std::literals;

namespace {
dom::Element const &body(dom::Document const &d) {
    return std::get<dom::Element>(d.html().children.at(1));
}
} // namespace

int main() {
    etest::Suite s{};

    s.add_test("doctype", [](etest::IActions &a) {
        auto document = html::parse("<!doctype html>"sv);
        a.expect(document.doctype == "html"s);
    });

    s.add_test("weirdly capitalized doctype", [](etest::IActions &a) {
        auto document = html::parse("<!docTYpe html>"sv);
        a.expect(document.doctype == "html"s);
    });

    s.add_test("everything is wrapped in a html element", [](etest::IActions &a) {
        auto document = html::parse("<p></p>"sv);
        auto html = document.html();
        a.expect(html.name == "html"s);
        a.require(html.children.size() == 2);
        a.expect(std::get<dom::Element>(html.children.at(0)).name == "head"s);

        auto const &body = std::get<dom::Element>(html.children.at(1));
        a.expect_eq(body.name, "body"s);

        a.expect_eq(std::get<dom::Element>(body.children.at(0)).name, "p");
    });

    s.add_test("single element", [](etest::IActions &a) {
        auto html = html::parse("<html></html>"sv).html();
        a.expect(html.name == "html"s);
        a.expect(html.attributes.empty());
    });

    s.add_test("element w/ attributes", [](etest::IActions &a) {
        auto html = html::parse("<body><p hello=world>"sv).html();
        a.expect_eq(html,
                dom::Element{"html",
                        {},
                        {dom::Element{"head"}, dom::Element{"body", {}, {dom::Element{"p", {{"hello", "world"}}}}}}});
    });

    s.add_test("element before body", [](etest::IActions &a) {
        auto html = html::parse("</head><p>"sv).html();
        a.expect_eq(
                html, dom::Element{"html", {}, {dom::Element{"head"}, dom::Element{"body", {}, {dom::Element{"p"}}}}});
    });

    s.add_test("eof before body", [](etest::IActions &a) {
        auto html = html::parse("</head>"sv).html();
        a.expect_eq(html, dom::Element{"html", {}, {dom::Element{"head"}, dom::Element{"body"}}});
    });

    s.add_test("script", [](etest::IActions &a) {
        auto html = html::parse("</head><script>console.log(13)"sv).html();
        dom::Element expected{
                "html",
                {},
                {
                        dom::Element{
                                "head",
                                {},
                                {dom::Element{"script", {}, {dom::Text{"console.log(13)"}}}},
                        },
                        dom::Element{"body"},
                },
        };
        a.expect_eq(html, expected);
    });

    s.add_test("self-closing single element", [](etest::IActions &a) {
        auto doc = html::parse("<br>"sv);

        auto const &br = std::get<dom::Element>(body(doc).children.at(0));
        a.expect(br.children.empty());
        a.expect(br.name == "br"s);
        a.expect(br.attributes.empty());
    });

    s.add_test("self-closing single element with slash", [](etest::IActions &a) {
        auto doc = html::parse("<img/>"sv);

        auto const &img = std::get<dom::Element>(body(doc).children.at(0));
        a.expect(img.children.empty());
        a.expect(img.name == "img"s);
        a.expect(img.attributes.empty());
    });

    s.add_test("multiple elements", [](etest::IActions &a) {
        auto doc = html::parse("<span></span><div></div>"sv);

        auto const &span = std::get<dom::Element>(body(doc).children.at(0));
        a.expect(span.children.empty());
        a.expect(span.name == "span"s);
        a.expect(span.attributes.empty());

        auto const &div = std::get<dom::Element>(body(doc).children.at(1));
        a.expect(div.children.empty());
        a.expect(div.name == "div"s);
        a.expect(div.attributes.empty());
    });

    s.add_test("nested elements", [](etest::IActions &a) {
        auto html = html::parse("<html><body></body></html>"sv).html();
        a.require(html.children.size() == 2);
        a.expect(html.name == "html"s);
        a.expect(html.attributes.empty());

        auto const &head = std::get<dom::Element>(html.children.at(0));
        a.expect(head.name == "head"s);
        a.expect(head.attributes.empty());

        auto const &body = std::get<dom::Element>(html.children.at(1));
        a.expect(body.name == "body"s);
        a.expect(body.attributes.empty());
    });

    s.add_test("single-quoted attribute", [](etest::IActions &a) {
        auto nodes = html::parse("<meta charset='utf-8'/>"sv).html().children;
        a.require(nodes.size() == 2);

        auto const &head = std::get<dom::Element>(nodes.at(0));
        auto const &meta = std::get<dom::Element>(head.children.at(0));
        a.expect(meta.children.empty());
        a.expect(meta.name == "meta"s);
        a.expect(meta.attributes.size() == 1);
        a.expect(meta.attributes.at("charset") == "utf-8"s);
    });

    s.add_test("double-quoted attribute", [](etest::IActions &a) {
        auto nodes = html::parse(R"(<meta charset="utf-8"/>)"sv).html().children;
        a.require(nodes.size() == 2);

        auto const &head = std::get<dom::Element>(nodes.at(0));
        auto const &meta = std::get<dom::Element>(head.children.at(0));
        a.expect(meta.children.empty());
        a.expect(meta.name == "meta"s);
        a.expect(meta.attributes.size() == 1);
        a.expect(meta.attributes.at("charset"s) == "utf-8"s);
    });

    s.add_test("multiple attributes", [](etest::IActions &a) {
        auto nodes = html::parse(R"(<meta name="viewport" content="width=100em, initial-scale=1"/>)"sv).html().children;
        a.require(nodes.size() == 2);

        auto const &head = std::get<dom::Element>(nodes.at(0));
        auto const &meta = std::get<dom::Element>(head.children.at(0));
        a.expect(meta.children.empty());
        a.expect(meta.name == "meta"s);
        a.expect(meta.attributes.size() == 2);
        a.expect(meta.attributes.at("name"s) == "viewport"s);
        a.expect(meta.attributes.at("content"s) == "width=100em, initial-scale=1"s);
    });

    s.add_test("multiple nodes with attributes", [](etest::IActions &a) {
        auto html = html::parse("<html bonus='hello'><body style='fancy'></body></html>"sv).html();
        a.require(html.children.size() == 2);
        a.expect(html.name == "html"s);
        a.expect(html.attributes.size() == 1);
        a.expect(html.attributes.at("bonus"s) == "hello"s);

        auto const &body = std::get<dom::Element>(html.children.at(1));
        a.expect(body.name == "body"s);
        a.expect(body.attributes.size() == 1);
        a.expect(body.attributes.at("style"s) == "fancy"s);
    });

    s.add_test("text node", [](etest::IActions &a) {
        auto html = html::parse("<html>fantastic, the future is now</html>"sv).html();
        a.expect_eq(html.children.size(), std::size_t{2});

        auto const &head = std::get<dom::Element>(html.children.at(0));
        a.expect_eq(head, dom::Element{.name = "head"});
        auto const &body = std::get<dom::Element>(html.children.at(1));
        a.expect_eq(body, dom::Element{.name = "body", .children{dom::Text{"fantastic, the future is now"}}});
    });

    s.add_test("character reference in attribute", [](etest::IActions &a) {
        auto html = html::parse("<html test='&lt;3'></html>"sv).html();
        a.expect(html.name == "html"s);
        a.expect(html.attributes.size() == 1);
        a.expect(html.attributes.at("test") == "<3");
    });

    s.add_test("character reference in attribute, no semicolon", [](etest::IActions &a) {
        auto html = html::parse("<html test='&lt3'></html>"sv).html();
        a.expect(html.name == "html"s);
        a.expect(html.attributes.size() == 1);
        a.expect(html.attributes.at("test") == "&lt3");
    });

    s.add_test("br shouldn't open a new scope", [](etest::IActions &a) {
        auto html = html::parse("<br><p></p>"sv).html();
        a.require(html.children.size() == 2);

        auto const &body = std::get<dom::Element>(html.children.at(1));

        auto const &br = std::get<dom::Element>(body.children.at(0));
        a.expect(br.name == "br"sv);
        a.expect(br.children.empty());

        auto const &p = std::get<dom::Element>(body.children.at(1));
        a.expect(p.name == "p"sv);
        a.expect(p.children.empty());
    });

    s.add_test("script is handled correctly", [](etest::IActions &a) {
        auto html = html::parse("<script><hello></script>"sv).html();
        a.require_eq(html.children.size(), std::size_t{2});

        auto const &head = std::get<dom::Element>(html.children.at(0));
        auto const &script = std::get<dom::Element>(head.children.at(0));
        a.expect_eq(script.name, "script"sv);
        a.expect_eq(script.children.size(), std::size_t{1});

        auto const &script_content = std::get<dom::Text>(script.children.at(0));
        a.expect_eq(script_content.text, "<hello>"sv);
    });

    s.add_test("special rules, p end tag omission", [](etest::IActions &a) {
        auto doc = html::parse("<html><p>hello<p>world</html>"sv);

        auto const &p1 = std::get<dom::Element>(body(doc).children.at(0));
        a.expect_eq(p1.name, "p");

        a.require_eq(p1.children.size(), std::size_t{1});
        auto const &p1_text = std::get<dom::Text>(p1.children.at(0));
        a.expect_eq(p1_text, dom::Text{"hello"});

        auto const &p2 = std::get<dom::Element>(body(doc).children.at(1));
        a.expect_eq(p2.name, "p");

        a.require_eq(p2.children.size(), std::size_t{1});
        auto const &p2_text = std::get<dom::Text>(p2.children.at(0));
        a.expect_eq(p2_text, dom::Text{"world"});
    });

    s.add_test("special rules, html tag omission", [](etest::IActions &a) {
        auto html = html::parse("<head></head><body>hello</body>"sv).html();
        a.require_eq(html.children.size(), std::size_t{2});

        auto const &head = std::get<dom::Element>(html.children.at(0));
        a.expect_eq(head.name, "head");

        auto const &body = std::get<dom::Element>(html.children.at(1));
        a.expect_eq(body.name, "body");

        a.require_eq(body.children.size(), std::size_t{1});
        auto const &body_text = std::get<dom::Text>(body.children.at(0));
        a.expect_eq(body_text, dom::Text{"hello"});
    });

    s.add_test("special rules, an empty string still parses as html", [](etest::IActions &a) {
        auto html = html::parse("").html();
        a.require_eq(html.children.size(), std::size_t{2});

        auto const &head = std::get<dom::Element>(html.children.at(0));
        a.expect_eq(head.name, "head");
        auto const &body = std::get<dom::Element>(html.children.at(1));
        a.expect_eq(body.name, "body");
    });

    s.add_test("special rules, just closing html is fine", [](etest::IActions &a) {
        auto html = html::parse("</html>").html();
        a.require_eq(html.children.size(), std::size_t{2});

        auto const &head = std::get<dom::Element>(html.children.at(0));
        a.expect_eq(head.name, "head");
        auto const &body = std::get<dom::Element>(html.children.at(1));
        a.expect_eq(body.name, "body");
    });

    s.add_test("special rules, head is auto-closed on html end tag", [](etest::IActions &a) {
        auto html = html::parse("<head></html>").html();
        a.require_eq(html.children.size(), std::size_t{2});

        auto const &head = std::get<dom::Element>(html.children.at(0));
        a.expect_eq(head.name, "head");
    });

    s.add_test("special rules, head tag omission", [](etest::IActions &a) {
        auto html = html::parse("<title>hello</title>"sv).html();

        auto const &head = std::get<dom::Element>(html.children.at(0));
        a.expect_eq(head.name, "head");

        auto const &title = std::get<dom::Element>(head.children.at(0));
        a.expect_eq(title.name, "title");
    });

    s.add_test("special rules, just text is fine too", [](etest::IActions &a) {
        auto html = html::parse("hello?"sv).html();
        a.require_eq(html.children.size(), std::size_t{2});

        auto const &head = std::get<dom::Element>(html.children.at(0));
        a.expect_eq(head.name, "head");

        auto const &body = std::get<dom::Element>(html.children.at(1));
        a.expect_eq(body.name, "body");

        a.require_eq(body.children.size(), std::size_t{1});
        auto const &body_text = std::get<dom::Text>(body.children.at(0));
        a.expect_eq(body_text, dom::Text{"hello?"});
    });

    s.add_test("<style> consumes everything as raw text", [](etest::IActions &a) {
        auto html = html::parse("<style><body>"sv).html();

        auto const &head = std::get<dom::Element>(html.children.at(0));
        a.expect_eq(head.name, "head");

        auto const &style = std::get<dom::Element>(head.children.at(0));
        a.expect_eq(style.name, "style");
        a.expect_eq(style.children.size(), std::size_t{1});

        auto const &text = std::get<dom::Text>(style.children.at(0));
        a.expect_eq(text.text, "<body>");
    });

    s.add_test("<noscript> w/ scripting consumes everything as raw text", [](etest::IActions &a) {
        auto html = html::parse("<body><noscript><span>"sv, {.scripting = true}).html();

        auto const &body = std::get<dom::Element>(html.children.at(1));

        a.expect_eq(body.name, "body");

        auto const &noscript = std::get<dom::Element>(body.children.at(0));
        a.expect_eq(noscript.name, "noscript");
        a.expect_eq(noscript.children.size(), std::size_t{1});

        auto const &text = std::get<dom::Text>(noscript.children.at(0));
        a.expect_eq(text.text, "<span>");
    });

    s.add_test("<noscript> w/o scripting is treated as a normal dom node", [](etest::IActions &a) {
        auto html = html::parse("<body><noscript><span>"sv, {.scripting = false}).html();

        auto const &body = std::get<dom::Element>(html.children.at(1));
        a.expect_eq(body.name, "body");

        auto const &noscript = std::get<dom::Element>(body.children.at(0));
        a.expect_eq(noscript.name, "noscript");
        a.expect_eq(noscript.children.size(), std::size_t{1});

        auto const &span = std::get<dom::Element>(noscript.children.at(0));
        a.expect_eq(span.name, "span");
    });

    s.add_test("end tag w/o any open elements", [](etest::IActions &a) {
        auto html = html::parse("</html></html>").html();
        a.expect_eq(html.children.size(), std::size_t{2});

        auto const &head = std::get<dom::Element>(html.children.at(0));
        a.expect_eq(head, dom::Element{"head"});

        auto const &body = std::get<dom::Element>(html.children.at(1));
        a.expect_eq(body, dom::Element{"body"});
    });

    s.add_test("mismatching end tag", [](etest::IActions &a) {
        auto html = html::parse("<p></a>").html();
        a.expect_eq(html.children.size(), std::size_t{2});

        auto const &head = std::get<dom::Element>(html.children.at(0));
        a.expect_eq(head, dom::Element{"head"});

        auto const &body = std::get<dom::Element>(html.children.at(1));
        a.expect_eq(body, dom::Element{"body", {}, {dom::Element{"p"}}});
    });

    s.add_test("comment", [](etest::IActions &a) {
        auto html = html::parse("</html><!-- hello -->").html();
        a.expect_eq(html.children.size(), std::size_t{2});

        auto const &head = std::get<dom::Element>(html.children.at(0));
        a.expect_eq(head, dom::Element{"head"});

        auto const &body = std::get<dom::Element>(html.children.at(1));
        a.expect_eq(body, dom::Element{"body"});
    });

    s.add_test("start tag after closing html", [](etest::IActions &a) {
        auto html = html::parse("</html><p></p>").html();
        a.expect_eq(html.children.size(), std::size_t{2});

        auto const &head = std::get<dom::Element>(html.children.at(0));
        a.expect_eq(head, dom::Element{"head"});

        auto const &body = std::get<dom::Element>(html.children.at(1));
        a.expect_eq(body, dom::Element{"body", {}, {dom::Element{"p"}}});
    });

    s.add_test("doctype", [](etest::IActions &a) {
        auto doc = html::parse(R"(<!doctype abcd PUBLIC "hello" "goodbye">)");
        a.expect_eq(doc.doctype, "abcd");
        a.expect_eq(doc.public_identifier, "hello");
        a.expect_eq(doc.system_identifier, "goodbye");
    });

    s.add_test("doctype, but too late!", [](etest::IActions &a) {
        auto doc = html::parse("<!doctype abcd></head><!doctype html>");
        a.expect_eq(doc.doctype, "abcd");
    });

    s.add_test("errors", [](etest::IActions &a) {
        auto errors = std::vector<html2::ParseError>{};
        std::ignore = html::parse("<!--", {}, [&](html2::ParseError e) { errors.push_back(e); });

        a.expect_eq(errors, std::vector{html2::ParseError::EofInComment});
    });

    s.add_test("errors, new api", [](etest::IActions &a) {
        auto errors = std::vector<html2::ParseError>{};
        html::Callbacks cbs{
                .on_error = [&](html2::ParseError e) { errors.push_back(e); },
        };

        std::ignore = html::parse("<!--", {}, cbs);

        a.expect_eq(errors, std::vector{html2::ParseError::EofInComment});
    });

    s.add_test("errors, new api, no cb set", [](etest::IActions &) {
        html::Callbacks cbs{};
        std::ignore = html::parse("<!--", {}, cbs);
    });

    s.add_test("on_element_closed cb", [](etest::IActions &a) {
        auto closed = std::vector<std::string>{};
        html::Callbacks cbs{
                .on_element_closed = [&](dom::Element const &e) { closed.push_back(e.name); },
        };

        auto doc = html::parse("<html><head></head><body></body></html>", {}, cbs);
        a.expect_eq(closed, std::vector<std::string>{"head", "body", "html"});
    });

    return s.run();
}
