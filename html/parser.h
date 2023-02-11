// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "dom/dom.h"
#include "html2/tokenizer.h"
#include "html/parse.h"

#include <functional>
#include <sstream>
#include <stack>
#include <string>
#include <string_view>
#include <utility>

namespace html {

class Parser {
public:
    [[nodiscard]] static std::tuple<dom::Document, std::vector<ExtResource>> parse_document(std::string_view input) {
        Parser parser{input};
        return parser.run();
    }

    // These must be public for std::visit to be happy with Parser as a visitor.
    void operator()(html2::DoctypeToken const &);
    void operator()(html2::StartTagToken const &);
    void operator()(html2::EndTagToken const &);
    void operator()(html2::CommentToken const &);
    void operator()(html2::CharacterToken const &);
    void operator()(html2::EndOfFileToken const &);

private:
    Parser(std::string_view input) : tokenizer_{input, std::bind_front(&Parser::on_token, this)} {}

    [[nodiscard]] std::tuple<dom::Document, std::vector<ExtResource>> run() {
        tokenizer_.run();
        return std::make_tuple(std::move(doc_), std::move(ext_resources_));
    }

    void on_token(html2::Tokenizer &, html2::Token &&token);
    void consume_tag_link(html2::StartTagToken const &);
    void generate_text_node_if_needed();

    html2::Tokenizer tokenizer_;
    dom::Document doc_{};
    std::vector<ExtResource> ext_resources_;
    std::stack<dom::Element *> open_elements_{};
    std::stringstream current_text_{};
    bool seen_html_tag_{false};
};

} // namespace html
