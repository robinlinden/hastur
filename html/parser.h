// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "dom/dom.h"
#include "html2/tokenizer.h"

#include <functional>
#include <sstream>
#include <stack>
#include <string_view>
#include <utility>

namespace html {

class Parser {
public:
    [[nodiscard]] static dom::Document parse_document(std::string_view input) {
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

    [[nodiscard]] dom::Document run() {
        tokenizer_.run();
        return std::move(doc_);
    }

    void on_token(html2::Tokenizer &, html2::Token &&token);

    void generate_text_node_if_needed();

    html2::Tokenizer tokenizer_;
    dom::Document doc_{};
    std::stack<dom::Element *> open_elements_{};
    std::stringstream current_text_{};
    bool seen_html_tag_{false};
};

} // namespace html
