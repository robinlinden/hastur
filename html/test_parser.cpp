#include "html/parse.h"

#include "etest/etest.h"

using namespace std::literals;
using etest::expect_true;

int main() {
    etest::test("doctype", [] {
        auto nodes = html::parse("<!doctype html>"sv);
        expect_true(nodes.size() == 1);

        auto doctype = nodes[0];
        expect_true(doctype.children.size() == 0);
        expect_true(std::get<dom::Doctype>(doctype.data).doctype == "html"s);
    });

    etest::test("weirdly capitalized doctype", [] {
        auto nodes = html::parse("<!docTYpe html>"sv);
        expect_true(nodes.size() == 1);

        auto doctype = nodes[0];
        expect_true(doctype.children.size() == 0);
        expect_true(std::get<dom::Doctype>(doctype.data).doctype == "html"s);
    });

    etest::test("single element", [] {
        auto nodes = html::parse("<html></html>"sv);
        expect_true(nodes.size() == 1);

        auto html = nodes[0];
        expect_true(html.children.size() == 0);
        expect_true(std::get<dom::Element>(html.data).name == "html"s);
        expect_true(std::get<dom::Element>(html.data).attributes.size() == 0);
    });

    etest::test("self-closing single element", [] {
        auto nodes = html::parse("<br>"sv);
        expect_true(nodes.size() == 1);

        auto br = nodes[0];
        expect_true(br.children.size() == 0);
        expect_true(std::get<dom::Element>(br.data).name == "br"s);
        expect_true(std::get<dom::Element>(br.data).attributes.size() == 0);
    });

    etest::test("self-closing single element with slash", [] {
        auto nodes = html::parse("<img/>"sv);
        expect_true(nodes.size() == 1);

        auto img = nodes[0];
        expect_true(img.children.size() == 0);
        expect_true(std::get<dom::Element>(img.data).name == "img"s);
        expect_true(std::get<dom::Element>(img.data).attributes.size() == 0);
    });

    etest::test("multiple elements", [] {
        auto nodes = html::parse("<span></span><div></div>"sv);
        expect_true(nodes.size() == 2);

        auto span = nodes[0];
        expect_true(span.children.size() == 0);
        expect_true(std::get<dom::Element>(span.data).name == "span"s);
        expect_true(std::get<dom::Element>(span.data).attributes.size() == 0);

        auto div = nodes[1];
        expect_true(div.children.size() == 0);
        expect_true(std::get<dom::Element>(div.data).name == "div"s);
        expect_true(std::get<dom::Element>(div.data).attributes.size() == 0);
    });

    etest::test("nested elements", [] {
        auto nodes = html::parse("<html><body></body></html>"sv);
        expect_true(nodes.size() == 1);

        auto html = nodes[0];
        expect_true(html.children.size() == 1);
        expect_true(std::get<dom::Element>(html.data).name == "html"s);
        expect_true(std::get<dom::Element>(html.data).attributes.size() == 0);

        auto body = html.children[0];
        expect_true(std::get<dom::Element>(body.data).name == "body"s);
        expect_true(std::get<dom::Element>(body.data).attributes.size() == 0);
    });

    etest::test("single-quoted attribute", [] {
        auto nodes = html::parse("<meta charset='utf-8'/>"sv);
        expect_true(nodes.size() == 1);

        auto meta = nodes[0];
        expect_true(meta.children.size() == 0);

        auto meta_data = std::get<dom::Element>(meta.data);
        expect_true(meta_data.name == "meta"s);
        expect_true(meta_data.attributes.size() == 1);
        expect_true(meta_data.attributes.at("charset") == "utf-8"s);
    });

    etest::test("double-quoted attribute", [] {
        auto nodes = html::parse("<meta charset=\"utf-8\"/>"sv);
        expect_true(nodes.size() == 1);

        auto meta = nodes[0];
        expect_true(meta.children.size() == 0);

        auto meta_data = std::get<dom::Element>(meta.data);
        expect_true(meta_data.name == "meta"s);
        expect_true(meta_data.attributes.size() == 1);
        expect_true(meta_data.attributes.at("charset"s) == "utf-8"s);
    });

    etest::test("multiple attributes", [] {
        auto nodes = html::parse("<meta name=\"viewport\" content=\"width=100em, initial-scale=1\"/>"sv);
        expect_true(nodes.size() == 1);

        auto meta = nodes[0];
        expect_true(meta.children.size() == 0);

        auto meta_data = std::get<dom::Element>(meta.data);
        expect_true(meta_data.name == "meta"s);
        expect_true(meta_data.attributes.size() == 2);
        expect_true(meta_data.attributes.at("name"s) == "viewport"s);
        expect_true(meta_data.attributes.at("content"s) == "width=100em, initial-scale=1"s);
    });

    etest::test("multiple nodes with attributes", [] {
        auto nodes = html::parse("<html bonus='hello'><body style='fancy'></body></html>"sv);
        expect_true(nodes.size() == 1);

        auto html = nodes[0];
        expect_true(html.children.size() == 1);
        auto html_data = std::get<dom::Element>(html.data);
        expect_true(html_data.name == "html"s);
        expect_true(html_data.attributes.size() == 1);
        expect_true(html_data.attributes.at("bonus"s) == "hello"s);

        auto body = html.children[0];
        auto body_data = std::get<dom::Element>(body.data);
        expect_true(body_data.name == "body"s);
        expect_true(body_data.attributes.size() == 1);
        expect_true(body_data.attributes.at("style"s) == "fancy"s);
    });

    etest::test("text node", [] {
        auto nodes = html::parse("<html>fantastic, the future is now</html>"sv);
        expect_true(nodes.size() == 1);

        auto html = nodes[0];
        expect_true(html.children.size() == 1);
        expect_true(std::get<dom::Element>(html.data).name == "html"s);
        expect_true(std::get<dom::Element>(html.data).attributes.size() == 0);

        auto text = html.children[0];
        expect_true(std::get<dom::Text>(text.data).text == "fantastic, the future is now"s);
    });

    return etest::run_all_tests();
}
