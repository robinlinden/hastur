#include "parser/parser.h"

#include <catch2/catch.hpp>

using namespace std::literals;

namespace {

TEST_CASE("parser", "[parser]") {
    using parser::Parser;

    SECTION("doctype") {
        Parser parser{"<!doctype html>"sv};
        auto nodes = parser.parse_nodes();
        REQUIRE(nodes.size() == 1);

        auto doctype = nodes[0];
        REQUIRE(doctype.children.size() == 0);
        REQUIRE(std::get<dom::Doctype>(doctype.data).doctype == "html"s);
    }

    SECTION("weirdly capitalized doctype") {
        Parser parser{"<!docTYpe html>"sv};
        auto nodes = parser.parse_nodes();
        REQUIRE(nodes.size() == 1);

        auto doctype = nodes[0];
        REQUIRE(doctype.children.size() == 0);
        REQUIRE(std::get<dom::Doctype>(doctype.data).doctype == "html"s);
    }

    SECTION("single element") {
        Parser parser{"<html></html>"sv};
        auto nodes = parser.parse_nodes();
        REQUIRE(nodes.size() == 1);

        auto html = nodes[0];
        REQUIRE(html.children.size() == 0);
        REQUIRE(std::get<dom::Element>(html.data).name == "html"s);
        REQUIRE(std::get<dom::Element>(html.data).attributes.size() == 0);
    }

    SECTION("self-closing single element") {
        Parser parser{"<br>"sv};
        auto nodes = parser.parse_nodes();
        REQUIRE(nodes.size() == 1);

        auto br = nodes[0];
        REQUIRE(br.children.size() == 0);
        REQUIRE(std::get<dom::Element>(br.data).name == "br"s);
        REQUIRE(std::get<dom::Element>(br.data).attributes.size() == 0);
    }

    SECTION("self-closing single element with slash") {
        Parser parser{"<img/>"sv};
        auto nodes = parser.parse_nodes();
        REQUIRE(nodes.size() == 1);

        auto img = nodes[0];
        REQUIRE(img.children.size() == 0);
        REQUIRE(std::get<dom::Element>(img.data).name == "img"s);
        REQUIRE(std::get<dom::Element>(img.data).attributes.size() == 0);
    }

    SECTION("multiple elements") {
        Parser parser{"<span></span><div></div>"sv};
        auto nodes = parser.parse_nodes();
        REQUIRE(nodes.size() == 2);

        auto span = nodes[0];
        REQUIRE(span.children.size() == 0);
        REQUIRE(std::get<dom::Element>(span.data).name == "span"s);
        REQUIRE(std::get<dom::Element>(span.data).attributes.size() == 0);

        auto div = nodes[1];
        REQUIRE(div.children.size() == 0);
        REQUIRE(std::get<dom::Element>(div.data).name == "div"s);
        REQUIRE(std::get<dom::Element>(div.data).attributes.size() == 0);
    }

    SECTION("nested elements") {
        Parser parser{"<html><body></body></html>"sv};
        auto nodes = parser.parse_nodes();
        REQUIRE(nodes.size() == 1);

        auto html = nodes[0];
        REQUIRE(html.children.size() == 1);
        REQUIRE(std::get<dom::Element>(html.data).name == "html"s);
        REQUIRE(std::get<dom::Element>(html.data).attributes.size() == 0);

        auto body = html.children[0];
        REQUIRE(std::get<dom::Element>(body.data).name == "body"s);
        REQUIRE(std::get<dom::Element>(body.data).attributes.size() == 0);
    }

    SECTION("single-quoted attribute") {
        Parser parser{"<meta charset='utf-8'/>"sv};
        auto nodes = parser.parse_nodes();
        REQUIRE(nodes.size() == 1);

        auto meta = nodes[0];
        REQUIRE(meta.children.size() == 0);

        auto meta_data = std::get<dom::Element>(meta.data);
        REQUIRE(meta_data.name == "meta"s);
        REQUIRE(meta_data.attributes.size() == 1);
        REQUIRE(meta_data.attributes.at("charset") == "utf-8"s);
    }

    SECTION("double-quoted attribute") {
        Parser parser{"<meta charset=\"utf-8\"/>"sv};
        auto nodes = parser.parse_nodes();
        REQUIRE(nodes.size() == 1);

        auto meta = nodes[0];
        REQUIRE(meta.children.size() == 0);

        auto meta_data = std::get<dom::Element>(meta.data);
        REQUIRE(meta_data.name == "meta"s);
        REQUIRE(meta_data.attributes.size() == 1);
        REQUIRE(meta_data.attributes.at("charset"s) == "utf-8"s);
    }

    SECTION("multiple attributes") {
        Parser parser{"<meta name=\"viewport\" content=\"width=100em, initial-scale=1\"/>"sv};
        auto nodes = parser.parse_nodes();
        REQUIRE(nodes.size() == 1);

        auto meta = nodes[0];
        REQUIRE(meta.children.size() == 0);

        auto meta_data = std::get<dom::Element>(meta.data);
        REQUIRE(meta_data.name == "meta"s);
        REQUIRE(meta_data.attributes.size() == 2);
        REQUIRE(meta_data.attributes.at("name"s) == "viewport"s);
        REQUIRE(meta_data.attributes.at("content"s) == "width=100em, initial-scale=1"s);
    }

    SECTION("multiple nodes with attributes") {
        Parser parser{"<html bonus='hello'><body style='fancy'></body></html>"sv};
        auto nodes = parser.parse_nodes();
        REQUIRE(nodes.size() == 1);

        auto html = nodes[0];
        REQUIRE(html.children.size() == 1);
        auto html_data = std::get<dom::Element>(html.data);
        REQUIRE(html_data.name == "html"s);
        REQUIRE(html_data.attributes.size() == 1);
        REQUIRE(html_data.attributes.at("bonus"s) == "hello"s);

        auto body = html.children[0];
        auto body_data = std::get<dom::Element>(body.data);
        REQUIRE(body_data.name == "body"s);
        REQUIRE(body_data.attributes.size() == 1);
        REQUIRE(body_data.attributes.at("style"s) == "fancy"s);
    }

    SECTION("text node") {
        Parser parser{"<html>fantastic, the future is now</html>"sv};
        auto nodes = parser.parse_nodes();
        REQUIRE(nodes.size() == 1);

        auto html = nodes[0];
        REQUIRE(html.children.size() == 1);
        REQUIRE(std::get<dom::Element>(html.data).name == "html"s);
        REQUIRE(std::get<dom::Element>(html.data).attributes.size() == 0);

        auto text = html.children[0];
        REQUIRE(std::get<dom::Text>(text.data).text == "fantastic, the future is now"s);
    }
}

} // namespace
