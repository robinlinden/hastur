// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef HTML_PARSER_H_
#define HTML_PARSER_H_

#include "html/parser_actions.h"
#include "html/parser_states.h"

#include "dom/dom.h"
#include "html2/tokenizer.h"

#include <functional>
#include <sstream>
#include <stack>
#include <string_view>
#include <utility>

namespace html {

struct ParserOptions {
    bool scripting{false};
};

class Parser {
public:
    [[nodiscard]] static dom::Document parse_document(std::string_view input, ParserOptions const &opts) {
        Parser parser{input, opts};
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
    Parser(std::string_view input, ParserOptions const &opts)
        : tokenizer_{input, std::bind_front(&Parser::on_token, this)}, scripting_{opts.scripting} {}

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
    bool scripting_{false};
    InsertionMode insertion_mode_{};
    Actions actions_{doc_, tokenizer_, scripting_, open_elements_};
};

inline dom::Document parse(std::string_view input, ParserOptions const &opts = {}) {
    return Parser::parse_document(input, opts);
}

} // namespace html

#endif
