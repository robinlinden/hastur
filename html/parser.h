// SPDX-FileCopyrightText: 2021-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef HTML_PARSER_H_
#define HTML_PARSER_H_

#include "html/parser_actions.h"

#include "dom/dom.h"
#include "html2/parser_states.h"
#include "html2/token.h"
#include "html2/tokenizer.h"

#include <functional>
#include <string_view>
#include <utility>
#include <vector>

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

private:
    Parser(std::string_view input, ParserOptions const &opts)
        : tokenizer_{input, std::bind_front(&Parser::on_token, this)}, scripting_{opts.scripting} {}

    [[nodiscard]] dom::Document run() {
        tokenizer_.run();
        return std::move(doc_);
    }

    void on_token(html2::Tokenizer &, html2::Token &&token);

    html2::Tokenizer tokenizer_;
    dom::Document doc_{};
    std::vector<dom::Element *> open_elements_{};
    bool scripting_{false};
    html2::InsertionMode insertion_mode_{};
    Actions actions_{doc_, tokenizer_, scripting_, insertion_mode_, open_elements_};
};

inline dom::Document parse(std::string_view input, ParserOptions const &opts = {}) {
    return Parser::parse_document(input, opts);
}

} // namespace html

#endif
