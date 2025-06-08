// SPDX-FileCopyrightText: 2021-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef HTML_PARSER_H_
#define HTML_PARSER_H_

#include "html/parser_actions.h"

#include "dom/dom.h"
#include "html2/parse_error.h"
#include "html2/parser_states.h"
#include "html2/token.h"
#include "html2/tokenizer.h"

#include <functional>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace html {

struct ParserOptions {
    bool scripting{false};
};

struct Callbacks {
    std::function<void(dom::Element const &)> on_element_closed;
    std::function<void(html2::ParseError)> on_error;
};

class Parser {
public:
    [[nodiscard]] static dom::Document parse_document(
            std::string_view input, ParserOptions const &opts, Callbacks const &cbs) {
        Parser parser{input, opts, cbs};
        return parser.run();
    }

private:
    Parser(std::string_view input, ParserOptions const &opts, Callbacks const &cbs)
        : tokenizer_{input,
                  [this](html2::Tokenizer &tokenizer, html2::Token &&token) { on_token(tokenizer, std::move(token)); },
                  [&cbs](html2::Tokenizer &, html2::ParseError err) {
                      if (!cbs.on_error) {
                          return;
                      }

                      cbs.on_error(err);
                  }},
          scripting_{opts.scripting}, cbs_{cbs} {}

    [[nodiscard]] dom::Document run() {
        tokenizer_.run();
        while (!open_elements_.empty()) {
            actions_.pop_current_node();
        }

        return std::move(doc_);
    }

    void on_token(html2::Tokenizer &, html2::Token &&token) {
        insertion_mode_ = std::visit([&](auto &mode) { return mode.process(actions_, token); }, insertion_mode_)
                                  .value_or(insertion_mode_);
    }

    html2::Tokenizer tokenizer_;
    dom::Document doc_{};
    std::vector<dom::Element *> open_elements_{};
    bool scripting_{false};
    Callbacks const &cbs_;
    html2::InsertionMode insertion_mode_{};
    Actions actions_{doc_, tokenizer_, scripting_, insertion_mode_, open_elements_, cbs_.on_element_closed};
};

inline dom::Document parse(std::string_view input, ParserOptions const &opts, Callbacks const &cbs) {
    return Parser::parse_document(input, opts, cbs);
}

inline dom::Document parse(
        std::string_view input,
        ParserOptions const &opts = {},
        std::function<void(html2::ParseError)> const &on_error = [](auto) {}) {
    return parse(input, opts, Callbacks{.on_error = on_error});
}

} // namespace html

#endif
