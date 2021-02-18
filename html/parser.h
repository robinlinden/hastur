#ifndef HTML_PARSER_H_
#define HTML_PARSER_H_

#include "dom/dom.h"

#include <array>
#include <cctype>
#include <cstddef>
#include <functional>
#include <string_view>
#include <utility>
#include <vector>

namespace html {

// Inspired by
// https://github.com/servo/rust-cssparser/blob/02129220f848246ce8899f45a50d4b15068ebd79/src/tokenizer.rs
class Parser {
public:
    Parser(std::string_view input) : input_{input} {}

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

private:
    // https://html.spec.whatwg.org/multipage/syntax.html#void-elements
    static constexpr auto void_elements = std::array{
            "area", "base", "br", "col", "embed",
            "hr", "img", "input", "link", "meta",
            "param", "source", "track", "wbr"};

    constexpr bool is_void_element(std::string_view tag) {
        return find(begin(void_elements), end(void_elements), tag) != end(void_elements);
    }

    constexpr char peek() const {
        return input_[pos_];
    }

    constexpr std::string_view peek(std::size_t chars) const {
        return input_.substr(pos_, chars);
    }

    constexpr bool starts_with(std::string_view prefix) const {
        return peek(prefix.size()) == prefix;
    }

    constexpr bool is_eof() const {
        return pos_ >= input_.size();
    }

    constexpr char consume_char() {
        return input_[pos_++];
    }

    constexpr void advance(std::size_t n) {
        pos_ += n;
    }

    std::string_view consume_while(std::function<bool(char)> const &pred) {
        std::size_t start = pos_;
        while (pred(input_[pos_])) { ++pos_; }
        return input_.substr(start, pos_ - start);
    }

    constexpr void skip_whitespace() {
        while (!is_eof() && std::isspace(static_cast<unsigned char>(peek()))) { advance(1); }
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

    dom::Node parse_doctype() {
        consume_while([](char c) { return c != ' '; }); // <!doctype
        skip_whitespace();
        auto doctype = dom::create_doctype_node(consume_while([](char c) { return c != '>'; }));
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
        using namespace std::string_view_literals;

        constexpr auto doctype_prefix = "<!doctype"sv;
        auto peeked = peek(doctype_prefix.size());
        if (no_case_compare(doctype_prefix, peeked)) { return parse_doctype(); }
        if (peek() == '<') { return parse_element(); }
        return parse_text();
    }

    std::string_view input_;
    std::size_t pos_{0};
};

} // namespace parser

#endif
