#ifndef HTML_PARSER_H_
#define HTML_PARSER_H_

#include "dom/dom.h"
#include "util/base_parser.h"

#include <array>
#include <cstddef>
#include <string_view>
#include <utility>
#include <vector>

namespace html {

class Parser final : util::BaseParser {
public:
    Parser(std::string_view input) : BaseParser{input} {}

    dom::Document parse_document() {
        using namespace std::string_view_literals;

        constexpr auto doctype_prefix = "<!doctype"sv;
        auto peeked = peek(doctype_prefix.size());
        auto doctype = [&]() {
            if (no_case_compare(doctype_prefix, peeked)) { return parse_doctype(); }
            return "quirks"sv;
        }();

        auto children = parse_nodes();
        if (children.size() == 1 && std::get<dom::Element>(children[0].data).name == "html") {
            return dom::create_document(doctype, std::move(children[0]));
        }

        return dom::create_document(doctype, dom::create_element_node("html", {}, std::move(children)));
    }

private:
    // https://html.spec.whatwg.org/multipage/syntax.html#void-elements
    static constexpr auto void_elements = std::array{
            "area", "base", "br", "col", "embed",
            "hr", "img", "input", "link", "meta",
            "param", "source", "track", "wbr"};

    constexpr bool is_void_element(std::string_view tag) {
        return find(begin(void_elements), end(void_elements), tag) != end(void_elements);
    }

    std::vector<dom::Node> parse_nodes() {
        using namespace std::string_view_literals;

        std::vector<dom::Node> nodes;
        while (!is_eof()) {
            skip_whitespace();
            if (is_eof() || starts_with("</"sv)) { break; }
            nodes.push_back(parse_node());
        }
        return nodes;
    }

    std::string_view parse_tag_name() {
        return consume_while([](char c) {
            return (c >= 'a' && c <= 'z')
                    || (c >= 'A' && c <= 'Z')
                    || (c >= '0' && c <= '9');
        });
    }

    std::string_view parse_attr_value() {
        auto open_quote = consume_char(); // ' or "
        auto value = consume_while([=](char c) { return c != open_quote; });
        consume_char(); // same as open_quote
        return value;
    }

    std::pair<std::string, std::string> parse_attr() {
        auto name = parse_tag_name();
        consume_char(); // =
        auto value = parse_attr_value();
        return {std::string{name}, std::string{value}};
    }

    dom::AttrMap parse_attributes() {
        dom::AttrMap attrs;
        while (true) {
            skip_whitespace();
            if (peek() == '>' || starts_with("/>")) { break; }
            attrs.insert(parse_attr());
        }

        return attrs;
    }

    std::string_view parse_doctype() {
        consume_while([](char c) { return c != ' '; }); // <!doctype
        skip_whitespace();
        auto doctype = consume_while([](char c) { return c != '>'; });
        consume_char(); // >
        return doctype;
    }

    dom::Node parse_text() {
        return dom::create_text_node(consume_while([](char c) { return c != '<'; }));
    }

    dom::Node parse_element() {
        consume_char(); // <
        auto name = parse_tag_name();
        auto attrs = parse_attributes();
        if (is_void_element(name)) {
            if (consume_char() == '/') { // optional / or >
                consume_char(); // >
            }
            return dom::create_element_node(name, std::move(attrs), {});
        } else {
            consume_char(); // >
        }

        auto children = parse_nodes();
        consume_char(); // <
        consume_char(); // /
        parse_tag_name();
        consume_char(); // >
        return dom::create_element_node(name, std::move(attrs), std::move(children));
    }

    static constexpr bool no_case_compare(std::string_view a, std::string_view b) {
        if (a.size() != b.size()) { return false; }
        for (size_t i = 0; i < a.size(); ++i) {
            if (std::tolower(a[i]) != std::tolower(b[i])) {
                return false;
            }
        }

        return true;
    }

    dom::Node parse_node() {
        if (peek() == '<') { return parse_element(); }
        return parse_text();
    }
};

} // namespace parser

#endif
